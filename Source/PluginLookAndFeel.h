#pragma once

#include <JuceHeader.h>

namespace KSKSKSColours {
    inline const juce::Colour background { 19, 21, 27 };
    inline const juce::Colour panel      { 29, 32, 40 };
    inline const juce::Colour accent     { 183, 242, 139 };
    inline const juce::Colour muted      { 153, 161, 176 };
    inline const juce::Colour border     { 58, 64, 75 };
    inline const juce::Colour dial       { 47, 51, 62 };
    inline const juce::Colour track      { 67, 73, 86 };
    inline const juce::Colour grid       { 48, 53, 64 };
}

class KSKSKSLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(
        juce::Graphics& g,
        int x, int y, int width, int height,
        float sliderPos,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider& slider
    ) override;

    void drawComboBox(
        juce::Graphics& g,
        int width,
        int height,
        bool isButtonDown,
        int buttonX,
        int buttonY,
        int buttonW,
        int buttonH,
        juce::ComboBox& box
    ) override;

    void fillTextEditorBackground(
        juce::Graphics& g,
        int width,
        int height,
        juce::TextEditor& editor
    ) override;

    void drawTextEditorOutline(
        juce::Graphics& g,
        int width,
        int height,
        juce::TextEditor& editor
    ) override;

    juce::Label* createSliderTextBox(juce::Slider& slider) override;
    
    juce::Rectangle<int> getTooltipBounds(
        const juce::String& tipText,
        juce::Point<int> screenPos,
        juce::Rectangle<int> parentArea
    ) override;
};