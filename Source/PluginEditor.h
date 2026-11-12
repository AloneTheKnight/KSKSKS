#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "PluginLookAndFeel.h"
#include "PluginControls.h"

class SidechainInfoIcon final :
    public juce::Component,
    public juce::SettableTooltipClient
{
public:
    SidechainInfoIcon();

    void paint(juce::Graphics& g) override;
};

class KSKSKSAudioProcessorEditor final :
    public juce::AudioProcessorEditor,
    public juce::KeyListener,
    private juce::Timer
{
public:
    explicit KSKSKSAudioProcessorEditor(KSKSKSAudioProcessor& processor);

    ~KSKSKSAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::unique_ptr<juce::ComponentTraverser>
        createKeyboardFocusTraverser() override;

    bool keyPressed(
        const juce::KeyPress& key,
        juce::Component* originatingComponent) override;

private:
    KSKSKSAudioProcessor& audioProcessor;
    KSKSKSLookAndFeel customLookAndFeel;

    juce::TooltipWindow tooltipWindow;
    SidechainInfoIcon sidechainInfoIcon;

    juce::ComboBox modeBox;
    juce::ComboBox divisionBox;

    ValueSlider depthSlider;
    ValueSlider curveSlider;
    ValueSlider attackSlider;
    ValueSlider releaseSlider;
    ValueSlider thresholdSlider;

    juce::Label depthLabel;
    juce::Label curveLabel;
    juce::Label attackLabel;
    juce::Label releaseLabel;
    juce::Label thresholdLabel;
    juce::Label modeLabel;
    juce::Label divisionLabel;
    juce::Label statusLabel;

    juce::Label gainReductionTitleLabel;
    juce::Label gainReductionValueLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ComboAttachment> modeAttachment;
    std::unique_ptr<ComboAttachment> divisionAttachment;

    std::unique_ptr<SliderAttachment> depthAttachment;
    std::unique_ptr<SliderAttachment> curveAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;
    std::unique_ptr<SliderAttachment> thresholdAttachment;

    float displayedSidechainLevel = 0.0f;

    void timerCallback() override;
    void focusField(int index);

    void setupSlider(ValueSlider& slider);

    void setupLabel(juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KSKSKSAudioProcessorEditor)
};