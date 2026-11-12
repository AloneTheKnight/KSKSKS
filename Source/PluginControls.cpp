#include "PluginControls.h"
#include "PluginLookAndFeel.h"

RoundedValueLabel::RoundedValueLabel() {
    setJustificationType(juce::Justification::centred);
    setEditable(true, true, false);
    setWantsKeyboardFocus(true);

    setColour(
        juce::Label::textColourId,
        juce::Colours::white
    );

    setColour(
        juce::Label::backgroundColourId,
        KSKSKSColours::background
    );

    setColour(
        juce::Label::outlineColourId,
        KSKSKSColours::border
    );
}

void RoundedValueLabel::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds()
        .toFloat()
        .reduced(0.5f);

    const auto* editor = getCurrentTextEditor();

    const bool focused = isBeingEdited()
        || (editor != nullptr && editor->hasKeyboardFocus(true));

    g.setColour(KSKSKSColours::background);
    g.fillRoundedRectangle(bounds, 6.0f);

    g.setColour(focused
                ? KSKSKSColours::accent
                : KSKSKSColours::border
    );

    g.drawRoundedRectangle(
        bounds,
        6.0f,
        focused ? 1.5f : 1.0f
    );

    g.setColour(findColour(juce::Label::textColourId));
    g.setFont(getFont());

    auto textBounds = getLocalBounds().reduced(4);
    textBounds.translate(-1, 0);

    g.drawFittedText(
        getText(),
        textBounds,
        juce::Justification::centred,
        1
    );
}

void RoundedValueLabel::editorShown(juce::TextEditor* editor) {
    editor->setJustification(juce::Justification::centred);
    editor->setIndents(0, 0);
    editor->setBorder(juce::BorderSize<int>(0));
    editor->addKeyListener(this);

    editor->onFocusLost = [this]() {
        if (isBeingEdited()) hideEditor(false);
    };
}

bool RoundedValueLabel::keyPressed(const juce::KeyPress& key, juce::Component*) {
    if (key.getKeyCode() == juce::KeyPress::returnKey) {
        if (auto* editor = getCurrentTextEditor())
            textEditorReturnKeyPressed(*editor);

        return true;
    }

    if (key.getKeyCode() == juce::KeyPress::tabKey){
        const bool backwards = key.getModifiers().isShiftDown();

        hideEditor(false);

        if (onTab) onTab(backwards);

        return true;
    }

    return false;
}

void RoundedValueLabel::textEditorReturnKeyPressed(juce::TextEditor&) {
    juce::Component::SafePointer<RoundedValueLabel> safeThis(this);

    hideEditor(false);

    juce::MessageManager::callAsync([safeThis]() {
        if (safeThis == nullptr || !safeThis->isShowing()) return;

        auto* topLevel = safeThis->getTopLevelComponent();

        if (topLevel != nullptr
            && !safeThis->isCurrentlyBlockedByAnotherModalComponent()
            && topLevel->hasKeyboardFocus(true)) {
            safeThis->showEditor();
        }
    });
}