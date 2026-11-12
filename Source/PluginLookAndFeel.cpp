#include "PluginLookAndFeel.h"
#include "PluginControls.h"
#include <cmath>

void KSKSKSLookAndFeel::drawRotarySlider(
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider&
) {
    const float size = static_cast<float>(juce::jmin(width, height) - 14);

    const float cx = x + width * 0.5f;
    const float cy = y + height * 0.5f;
    const float radius = size * 0.5f;

    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    g.setColour(KSKSKSColours::dial);
    g.fillEllipse(cx - radius, cy - radius, size, size);

    juce::Path track;
    track.addCentredArc(
        cx, cy,
        radius - 4.0f, radius - 4.0f,
        0.0f,
        rotaryStartAngle,
        rotaryEndAngle,
        true
    );

    g.setColour(KSKSKSColours::track);
    g.strokePath(
        track,
        juce::PathStrokeType(
            5.0f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded
    ));

    juce::Path valueArc;
    valueArc.addCentredArc(
        cx, cy,
        radius - 4.0f, radius - 4.0f,
        0.0f,
        rotaryStartAngle,
        angle,
        true
    );

    g.setColour(KSKSKSColours::accent);
    g.strokePath(
        valueArc,
        juce::PathStrokeType(
            5.0f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded
    ));

    const float pointerLength = radius * 0.60f;
    const float px = cx + std::sin(angle) * pointerLength;
    const float py = cy - std::cos(angle) * pointerLength;

    g.setColour(juce::Colours::white);
    g.drawLine(cx, cy, px, py, 3.0f);
    g.fillEllipse(cx - 3.5f, cy - 3.5f, 7.0f, 7.0f);
}

void KSKSKSLookAndFeel::drawComboBox(
    juce::Graphics& g,
    int width,
    int height,
    bool isButtonDown,
    int buttonX,
    int buttonY,
    int buttonW,
    int buttonH,
    juce::ComboBox& box
) {
    const auto bounds = juce::Rectangle<float>(
        1.0f,
        1.0f,
        static_cast<float>(width) - 2.0f,
        static_cast<float>(height) - 2.0f
    );

    g.setColour(KSKSKSColours::accent.withAlpha(isButtonDown ? 0.13f : 0.055f));

    g.fillRoundedRectangle(bounds, 8.0f);

    g.setColour(KSKSKSColours::accent.withAlpha(box.hasKeyboardFocus(true) ? 0.75f : 0.30f));

    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

    const float cx = buttonX + buttonW * 0.5f;
    const float cy = buttonY + buttonH * 0.5f;

    juce::Path arrow;
    arrow.startNewSubPath(cx - 4.0f, cy - 2.0f);
    arrow.lineTo(cx, cy + 2.0f);
    arrow.lineTo(cx + 4.0f, cy - 2.0f);

    g.setColour(KSKSKSColours::accent);
    g.strokePath(
        arrow,
        juce::PathStrokeType(
            1.6f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded
    ));
}

void KSKSKSLookAndFeel::fillTextEditorBackground(
    juce::Graphics& g,
    int width,
    int height,
    juce::TextEditor&
) {
    const auto bounds = juce::Rectangle<float>(
        0.5f,
        0.5f,
        static_cast<float>(width) - 1.0f,
        static_cast<float>(height) - 1.0f
    );

    g.setColour(KSKSKSColours::background);
    g.fillRoundedRectangle(bounds, 6.0f);
}

void KSKSKSLookAndFeel::drawTextEditorOutline(
    juce::Graphics& g,
    int width,
    int height,
    juce::TextEditor& editor
) {
    const auto bounds = juce::Rectangle<float>(
        0.5f,
        0.5f,
        static_cast<float>(width) - 1.0f,
        static_cast<float>(height) - 1.0f
    );

    const bool focused = editor.hasKeyboardFocus(true);

    g.setColour(focused
                ? KSKSKSColours::accent
                : KSKSKSColours::border);

    g.drawRoundedRectangle(
        bounds,
        6.0f,
        focused ? 1.5f : 1.0f);
}

juce::Label* KSKSKSLookAndFeel::createSliderTextBox(juce::Slider&) {
    auto* label = new RoundedValueLabel();

    label->setJustificationType(juce::Justification::centred);
    label->setColour(juce::Label::textColourId, juce::Colours::white);
    label->setFont(juce::Font(12.0f));

    return label;
}

juce::Rectangle<int> KSKSKSLookAndFeel::getTooltipBounds(
    const juce::String& tipText,
    juce::Point<int> screenPos,
    juce::Rectangle<int> parentArea
) {
    auto bounds = juce::LookAndFeel_V2::getTooltipBounds(
        tipText,
        screenPos,
        parentArea
    );

    bounds = bounds.expanded(3, 3);

    return bounds;
}