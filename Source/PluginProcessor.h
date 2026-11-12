#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <cstdint>

class KSKSKSAudioProcessor final : public juce::AudioProcessor {
public:
    KSKSKSAudioProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock(
        juce::AudioBuffer<float>& buffer,
        juce::MidiBuffer& midiMessages
    ) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "KSKSKS"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }

    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destination) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

    float getSidechainInputLevel() const noexcept {
        return sidechainInputLevel.load(std::memory_order_relaxed);
    }

    float getGainReductionDb() const noexcept {
        return gainReductionDb.load(std::memory_order_relaxed);
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();

    enum class EnvelopePhase {
        idle,
        attack,
        release
    };

    double currentSampleRate = 44100.0;
    double fallbackPpq = 0.0;

    float detectorEnvelope = 0.0f;
    int samplesSinceKick = 0;

    EnvelopePhase envelopePhase = EnvelopePhase::idle;
    int envelopePosition = 0;
    int envelopeLength = 1;

    float envelopeStartGain = 1.0f;
    float duckGain = 1.0f;

    float smoothedDuckGain = 1.0f;

    std::int64_t lastSyncCycle = 0;
    bool syncCycleInitialized = false;

    int previousMode = -1;

    std::atomic<float> sidechainInputLevel{ 0.0f };

    std::atomic<float> gainReductionDb{ 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KSKSKSAudioProcessor)
};