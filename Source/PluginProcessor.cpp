#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <cstdint>

juce::AudioProcessorValueTreeState::ParameterLayout
KSKSKSAudioProcessor::createParameterLayout() {
    using namespace juce;

    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<AudioParameterChoice>(
        "MODE", "Mode",
        StringArray { "SYNC", "KICK" }, 0
    ));

    layout.add(std::make_unique<AudioParameterFloat>(
        "DEPTH", "Depth",
        NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.65f
    ));

    layout.add(std::make_unique<AudioParameterChoice>(
        "DIVISION", "Sync Division",
        StringArray {
            "1/1", "1/2", "1/4",
            "1/8", "1/16", "1/8T"
        }, 1)
    );

    layout.add(std::make_unique<AudioParameterFloat>(
        "CURVE", "Curve",
        NormalisableRange<float>(0.5f, 8.0f, 0.01f),
        3.0f));

    layout.add(std::make_unique<AudioParameterFloat>(
        "ATTACK", "Attack (ms)",
        NormalisableRange<float>(0.1f, 30.0f, 0.1f),
        2.0f));

    layout.add(std::make_unique<AudioParameterFloat>(
        "RELEASE", "Release (ms)",
        NormalisableRange<float>(20.0f, 1500.0f, 1.0f),
        250.0f));

    layout.add(std::make_unique<AudioParameterFloat>(
        "THRESHOLD", "Kick Threshold",
        NormalisableRange<float>(0.001f, 0.5f, 0.001f, 0.5f),
        0.03f));

    return layout;
}

KSKSKSAudioProcessor::KSKSKSAudioProcessor() :
    AudioProcessor(
        BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)
            .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)
    ),
    parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{}

void KSKSKSAudioProcessor::prepareToPlay(double sampleRate, int) {
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;

    fallbackPpq = 0.0;
    detectorEnvelope = 0.0f;
    samplesSinceKick = 0;

    envelopePhase = EnvelopePhase::idle;
    envelopePosition = 0;
    envelopeLength = 1;
    envelopeStartGain = 1.0f;
    duckGain = 1.0f;
    smoothedDuckGain = 1.0f;

    lastSyncCycle = 0;
    syncCycleInitialized = false;
    previousMode = -1;

    sidechainInputLevel.store(0.0f, std::memory_order_relaxed);
    gainReductionDb.store(0.0f, std::memory_order_relaxed);
}

void KSKSKSAudioProcessor::releaseResources() {}

void KSKSKSAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&
) {
    juce::ScopedNoDenormals noDenormals;

    auto main = getBusBuffer(buffer, true, 0);

    const bool sidechainAvailable = getBusCount(true) > 1
        && getBus(true, 1) != nullptr
        && getBus(true, 1)->isEnabled();

    juce::AudioBuffer<float> sidechain;

    if (sidechainAvailable) sidechain = getBusBuffer(buffer, true, 1);

    float inputLevel = 0.0f;

    if (sidechainAvailable) {
        for (int channel = 0; channel < sidechain.getNumChannels(); ++channel) {
            for (int sample = 0; sample < sidechain.getNumSamples(); ++sample) {
                inputLevel = juce::jmax(
                    inputLevel,
                    std::abs(sidechain.getSample(channel, sample))
                );
            }
        }
    }

    sidechainInputLevel.store(inputLevel, std::memory_order_relaxed);

    const int mode = static_cast<int>(parameters.getRawParameterValue("MODE")->load());

    const float depth = juce::jlimit(
        0.0f, 1.0f,
        parameters.getRawParameterValue("DEPTH")->load()
    );

    const int division = static_cast<int>(parameters.getRawParameterValue("DIVISION")->load());

    const float curve = juce::jlimit(
        0.5f, 8.0f,
        parameters.getRawParameterValue("CURVE")->load()
    );

    const float attackMs = parameters.getRawParameterValue("ATTACK")->load();

    const float releaseMs = parameters.getRawParameterValue("RELEASE")->load();

    const float threshold = parameters.getRawParameterValue("THRESHOLD")->load();

    const float minimumGain = depth >= 0.9999f ? 0.0f : 1.0f - depth;

    if (mode != previousMode) {
        previousMode = mode;
        envelopePhase = EnvelopePhase::idle;
        envelopePosition = 0;
        duckGain = 1.0f;
        smoothedDuckGain = 1.0f;
        envelopeStartGain = 1.0f;

        syncCycleInitialized = false;
        detectorEnvelope = 0.0f;
        samplesSinceKick = 0;
    }

    double bpm = 120.0;
    double startPpq = fallbackPpq;

    if (auto* processorPlayHead = getPlayHead()) {
        if (auto position = processorPlayHead->getPosition()) {
            if (auto hostBpm = position->getBpm()) bpm = *hostBpm;
            
            if (auto hostPpq = position->getPpqPosition()) startPpq = *hostPpq;
        }
    }

    if (!std::isfinite(bpm) || bpm < 1.0 || bpm > 1000.0) bpm = 120.0;

    const double ppqPerSample = bpm / (60.0 * currentSampleRate);

    constexpr double multipliers[] = {0.25, 0.5, 1.0, 2.0, 4.0, 3.0};

    const double multiplier = multipliers[juce::jlimit(0, 5, division)];

    const int attackSamples = juce::jmax(
        1,
        static_cast<int>(std::ceil(attackMs * 0.001 * currentSampleRate))
    );

    const int releaseSamples = juce::jmax(
        1,
        static_cast<int>(std::ceil(releaseMs * 0.001 * currentSampleRate))
    );

    const float detectorAttackCoeff = static_cast<float>(std::exp(-1.0 / (0.005 * currentSampleRate)));

    const float detectorReleaseCoeff = static_cast<float>(std::exp(-1.0 / (0.080 * currentSampleRate)));

    const int kickHoldoffSamples = juce::jmax(
        1,
        static_cast<int>(0.035 * currentSampleRate)
    );

    const auto startAttack = [&]() {
        envelopePhase = EnvelopePhase::attack;
        envelopePosition = 0;
        envelopeStartGain = duckGain;

        if (mode == 0) {
            const float totalGainRange = 1.0f - minimumGain;

            if (totalGainRange > 0.000001f) {
                const float remainingGainRange = juce::jlimit(
                    0.0f,
                    totalGainRange,
                    duckGain - minimumGain
                );

                const float remainingFraction = remainingGainRange / totalGainRange;

                envelopeLength = juce::jmax(
                    1,
                    static_cast<int>(std::ceil(attackSamples * remainingFraction))
                );
            } else {
                envelopeLength = attackSamples;
            }
        } else {
            envelopeLength = attackSamples;
        }
    };

    const float gainSmoothingCoeff = static_cast<float>(
        std::exp(-1.0 / (0.001 * currentSampleRate))
    );

    for (int sample = 0; sample < main.getNumSamples(); ++sample) {
        bool trigger = false;

        if (mode == 0) {
            const double currentPpq = startPpq + sample * ppqPerSample;

            const auto currentCycle = static_cast<std::int64_t>(
                std::floor(currentPpq * multiplier)
            );

            const bool newCycle = !syncCycleInitialized || currentCycle != lastSyncCycle;

            if (newCycle) {
                syncCycleInitialized = true;
                lastSyncCycle = currentCycle;
                trigger = true;
            }
        } else {
            if (samplesSinceKick < kickHoldoffSamples) ++samplesSinceKick;

            float peak = 0.0f;

            if (sidechainAvailable) {
                for (int channel = 0; channel < sidechain.getNumChannels(); ++channel) {
                    peak = juce::jmax(
                        peak,
                        std::abs(sidechain.getSample(channel, sample))
                    );
                }
            }

            trigger = sidechainAvailable
                && peak >= threshold
                && peak > detectorEnvelope * 1.5f
                && samplesSinceKick >= kickHoldoffSamples;

            const float detectorCoeff = peak > detectorEnvelope
                    ? detectorAttackCoeff
                    : detectorReleaseCoeff;

            detectorEnvelope = peak + detectorCoeff * (detectorEnvelope - peak);

            if (trigger) samplesSinceKick = 0;
        }

        if (trigger) startAttack();

        if (envelopePhase == EnvelopePhase::attack) {
            ++envelopePosition;

            const float progress = juce::jlimit(
                0.0f, 1.0f,
                static_cast<float>(envelopePosition) / static_cast<float>(envelopeLength));

            const float shaped = progress * progress * (3.0f - 2.0f * progress);

            duckGain = envelopeStartGain + (minimumGain - envelopeStartGain) * shaped;

            if (envelopePosition >= envelopeLength) {
                duckGain = minimumGain;

                envelopePhase = EnvelopePhase::release;
                envelopePosition = 0;
                envelopeLength = releaseSamples;
                envelopeStartGain = duckGain;
            }
        } else if (envelopePhase == EnvelopePhase::release) {
            ++envelopePosition;

            const float progress = juce::jlimit(
                0.0f, 1.0f,
                static_cast<float>(envelopePosition) / static_cast<float>(envelopeLength)
            );

            const float smooth = progress * progress * (3.0f - 2.0f * progress);

            const float shaped = std::pow(smooth, curve);

            duckGain = envelopeStartGain + (1.0f - envelopeStartGain) * shaped;

            if (envelopePosition >= envelopeLength) {
                duckGain = 1.0f;
                envelopePhase = EnvelopePhase::idle;
                envelopePosition = 0;
            }
        } else {
            duckGain = 1.0f;
        }

        smoothedDuckGain = duckGain + gainSmoothingCoeff * (smoothedDuckGain - duckGain);

        for (int channel = 0; channel < main.getNumChannels(); ++channel) {
            main.setSample(
                channel,
                sample,
                main.getSample(channel, sample) * smoothedDuckGain
            );
        }
    }

    const float safeGain = juce::jmax(smoothedDuckGain, 0.000001f);

    const float reductionDb = 20.0f * std::log10(safeGain);

    gainReductionDb.store(
        juce::jlimit(-120.0f, 0.0f, reductionDb),
        std::memory_order_relaxed
    );

    fallbackPpq = startPpq + main.getNumSamples() * ppqPerSample;
}

juce::AudioProcessorEditor*
KSKSKSAudioProcessor::createEditor() {
    return new KSKSKSAudioProcessorEditor(*this);
}

void KSKSKSAudioProcessor::getStateInformation(juce::MemoryBlock& destination) {
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destination);
}

void KSKSKSAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));

    if (xml && xml->hasTagName(parameters.state.getType())) {
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new KSKSKSAudioProcessor();
}