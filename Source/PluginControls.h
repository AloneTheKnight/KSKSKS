#pragma once

#include <JuceHeader.h>
#include <functional>

class ValueSlider : public juce::Slider {
public:
    float defaultValue = 0.0f;

    void mouseDown(const juce::MouseEvent& event) override {
        if (event.mods.isRightButtonDown()) {
            setValue(defaultValue, juce::sendNotificationSync);
            return;
        }

        juce::Slider::mouseDown(event);
    }
};

class RoundedValueLabel final :
    public juce::Label,
    public juce::KeyListener
{
public:
    std::function<void(bool)> onTab;

    RoundedValueLabel();

    void paint(juce::Graphics& g) override;
    void editorShown(juce::TextEditor* editor) override;

    bool keyPressed(
        const juce::KeyPress& key,
        juce::Component* originatingComponent) override;

    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
};