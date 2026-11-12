#include "PluginEditor.h"

#include <algorithm>
#include <utility>
#include <vector>

SidechainInfoIcon::SidechainInfoIcon() {
    setTooltip(
        "SIDECHAIN SETUP\n\n"
        "1. Open the KSKSKS plugin.\n"
        "2. Click Detailed settings.\n"
        "3. Open Plugin options.\n"
        "4. Select the Processing tab.\n"
        "5. Find the Sidechain input routing section "
        "and choose the Mixer Insert carrying your Kick.\n\n"
        "IMPORTANT: Select the Mixer Insert, not the Kick "
        "sample in the Channel Rack.\n\n"
        "The indicator lights up when audio reaches "
        "the Sidechain input."
    );

    setWantsKeyboardFocus(false);
}

void SidechainInfoIcon::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat();

    const auto diameter = juce::jmin(
        bounds.getWidth(),
        bounds.getHeight()
    ) - 1.0f;

    const auto circle = juce::Rectangle<float>(
        bounds.getCentreX() - diameter * 0.5f,
        bounds.getCentreY() - diameter * 0.5f,
        diameter,
        diameter
    );

    const auto colour = isMouseOver()
        ? KSKSKSColours::accent
        : KSKSKSColours::muted;

    g.setColour(colour);
    g.drawEllipse(circle, 1.3f);

    g.setFont(juce::Font(7.0f, juce::Font::bold));
    g.drawText("i", getLocalBounds(), juce::Justification::centred);
}

namespace {
    juce::Label* findSliderTextBox(juce::Slider& slider) {
        for (int i = 0; i < slider.getNumChildComponents(); ++i) {
            if (auto* label = dynamic_cast<juce::Label*>(slider.getChildComponent(i))) {
                return label;
            }
        }

        return nullptr;
    }

    class ValueFieldFocusTraverser final : public juce::ComponentTraverser {
    public:
        explicit ValueFieldFocusTraverser(std::vector<juce::Component*> components)
            : fields(std::move(components)) {}

        juce::Component* getDefaultComponent(juce::Component*) override {
            return fields.empty() ? nullptr : fields.front();
        }

        juce::Component* getNextComponent(juce::Component* current) override {
            return getAdjacent(current, 1);
        }

        juce::Component* getPreviousComponent(juce::Component* current) override {
            return getAdjacent(current, -1);
        }

        std::vector<juce::Component*> getAllComponents(juce::Component*) override {
            return fields;
        }

    private:
        std::vector<juce::Component*> fields;

        juce::Component* getAdjacent(
            juce::Component* current,
            int direction
        ) {
            auto* component = current;

            while (component != nullptr) {
                for (size_t i = 0; i < fields.size(); ++i) {
                    if (fields[i] == component) {
                        const int count = static_cast<int>(fields.size());

                        const int next = (static_cast<int>(i) + direction + count) % count;

                        return fields[static_cast<size_t>(next)];
                    }
                }

                component = component->getParentComponent();
            }

            return getDefaultComponent(nullptr);
        }
    };
}

KSKSKSAudioProcessorEditor::KSKSKSAudioProcessorEditor(KSKSKSAudioProcessor& processor) :
    AudioProcessorEditor(&processor),
    audioProcessor(processor),
    tooltipWindow(this, 700)
{
    setLookAndFeel(&customLookAndFeel);

    setFocusContainerType(juce::Component::FocusContainerType::keyboardFocusContainer);

    setSize(760, 500);

    auto& params = audioProcessor.parameters;

    for (auto* box : {&modeBox, &divisionBox})
    {
        box->setColour(
            juce::ComboBox::textColourId,
            juce::Colours::white
        );

        box->setColour(
            juce::ComboBox::backgroundColourId,
            KSKSKSColours::background.withAlpha(0.65f)
        );

        box->setColour(
            juce::ComboBox::outlineColourId,
            juce::Colours::transparentBlack
        );

        box->setWantsKeyboardFocus(true);
        box->addKeyListener(this);
    }

    modeBox.addItem("SYNC", 1);
    modeBox.addItem("KICK", 2);
    addAndMakeVisible(modeBox);

    divisionBox.addItem("1/1", 1);
    divisionBox.addItem("1/2", 2);
    divisionBox.addItem("1/4", 3);
    divisionBox.addItem("1/8", 4);
    divisionBox.addItem("1/16", 5);
    divisionBox.addItem("1/8T", 6);
    addAndMakeVisible(divisionBox);

    addAndMakeVisible(sidechainInfoIcon);

    modeAttachment = std::make_unique<ComboAttachment>(params, "MODE", modeBox);

    divisionAttachment = std::make_unique<ComboAttachment>(params, "DIVISION", divisionBox);

    setupSlider(depthSlider);
    setupSlider(curveSlider);
    setupSlider(attackSlider);
    setupSlider(releaseSlider);
    setupSlider(thresholdSlider);

    ValueSlider* focusSliders[] = {
        &depthSlider,
        &curveSlider,
        &attackSlider,
        &releaseSlider,
        &thresholdSlider
    };

    for (int i = 0; i < 5; ++i) {
        if (auto* label = dynamic_cast<RoundedValueLabel*>(findSliderTextBox(*focusSliders[i]))) {
            label->onTab = [this, i](bool backwards) {
                focusField(i + 2 + (backwards ? -1 : 1));
            };
        }
    }

    depthSlider.setRange(0.0, 1.0, 0.001);
    // depthSlider.setTextValueSuffix("%");

    depthSlider.textFromValueFunction = [](double value) {
        return juce::String(juce::roundToInt(value * 100.0)) + "%";
    };

    depthSlider.valueFromTextFunction = [](const juce::String& text) -> double {
        return juce::jlimit(
            0.0,
            1.0,
            text.retainCharacters("0123456789.").getFloatValue() / 100.0
        );
    };

    curveSlider.setRange(0.5, 8.0, 0.01);
    curveSlider.setNumDecimalPlacesToDisplay(2);

    curveSlider.onValueChange = [this]() {
        repaint();
    };

    attackSlider.setRange(0.1, 30.0, 0.1);
    attackSlider.setTextValueSuffix(" ms");

    releaseSlider.setRange(20.0, 1500.0, 1.0);
    releaseSlider.setTextValueSuffix(" ms");

    thresholdSlider.setRange(0.001, 0.5, 0.001);
    thresholdSlider.setNumDecimalPlacesToDisplay(3);

    depthSlider.defaultValue = 0.65f;
    curveSlider.defaultValue = 3.0f;
    attackSlider.defaultValue = 2.0f;
    releaseSlider.defaultValue = 250.0f;
    thresholdSlider.defaultValue = 0.03f;

    depthAttachment = std::make_unique<SliderAttachment>(
        params, "DEPTH", depthSlider);

    curveAttachment = std::make_unique<SliderAttachment>(
        params, "CURVE", curveSlider);

    attackAttachment = std::make_unique<SliderAttachment>(
        params, "ATTACK", attackSlider);

    releaseAttachment = std::make_unique<SliderAttachment>(
        params, "RELEASE", releaseSlider);

    thresholdAttachment = std::make_unique<SliderAttachment>(
        params, "THRESHOLD", thresholdSlider);

    setupLabel(depthLabel, "DEPTH");
    setupLabel(curveLabel, "CURVE");
    setupLabel(attackLabel, "ATTACK");
    setupLabel(releaseLabel, "RELEASE");
    setupLabel(thresholdLabel, "KICK THRESHOLD");
    setupLabel(modeLabel, "MODE");
    setupLabel(divisionLabel, "SYNC DIVISION");

    statusLabel.setJustificationType(juce::Justification::centredRight);

    statusLabel.setColour(juce::Label::textColourId, KSKSKSColours::muted);

    statusLabel.setFont(juce::Font(11.0f));
    addAndMakeVisible(statusLabel);

    gainReductionTitleLabel.setText(
        "GR:",
        juce::dontSendNotification);

    gainReductionTitleLabel.setJustificationType(
        juce::Justification::centredLeft);

    gainReductionTitleLabel.setColour(
        juce::Label::textColourId,
        KSKSKSColours::muted);

    gainReductionTitleLabel.setFont(
        juce::Font(11.0f, juce::Font::bold));

    addAndMakeVisible(gainReductionTitleLabel);

    gainReductionValueLabel.setText(
        "0.0 dB",
        juce::dontSendNotification);

    gainReductionValueLabel.setJustificationType(
        juce::Justification::centredRight);

    gainReductionValueLabel.setColour(
        juce::Label::textColourId,
        KSKSKSColours::accent);

    gainReductionValueLabel.setFont(
        juce::Font(11.0f, juce::Font::bold));

    addAndMakeVisible(gainReductionValueLabel);

    modeBox.onChange = [this]() {
        const bool isSync = modeBox.getSelectedId() == 1;
        const bool isKick = !isSync;

        divisionLabel.setVisible(isSync);
        divisionBox.setVisible(isSync);

        sidechainInfoIcon.setVisible(isKick);

        thresholdLabel.setVisible(isKick);
        thresholdSlider.setVisible(isKick);

        statusLabel.setText(
            isSync
                ? "BPM-synchronised ducking"
                : "Sidechain mode",
            juce::dontSendNotification);

        repaint();
    };

    modeBox.onChange();

    startTimerHz(30);
}

KSKSKSAudioProcessorEditor::~KSKSKSAudioProcessorEditor() {
    stopTimer();

    depthSlider.setLookAndFeel(nullptr);
    curveSlider.setLookAndFeel(nullptr);
    attackSlider.setLookAndFeel(nullptr);
    releaseSlider.setLookAndFeel(nullptr);
    thresholdSlider.setLookAndFeel(nullptr);

    setLookAndFeel(nullptr);
}

void KSKSKSAudioProcessorEditor::timerCallback() {
    const float target = audioProcessor.getSidechainInputLevel();

    const float smoothing = target > displayedSidechainLevel ? 0.55f : 0.18f;

    displayedSidechainLevel += (target - displayedSidechainLevel) * smoothing;

    const float reductionDb = audioProcessor.getGainReductionDb();

    const juce::String reductionText = reductionDb <= -119.9f
                                       ? "-INF dB"
                                       : juce::String(reductionDb, 1) + " dB";

    gainReductionValueLabel.setText(
        reductionText,
        juce::dontSendNotification);

    repaint();
}

void KSKSKSAudioProcessorEditor::setupSlider(ValueSlider& slider) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);

    slider.setLookAndFeel(&customLookAndFeel);

    slider.setTextBoxStyle(
        juce::Slider::TextBoxBelow,
        false,
        72,
        18
    );

    slider.setTextBoxIsEditable(true);
    slider.setDoubleClickReturnValue(false, 0.0);

    slider.setColour(
        juce::Slider::textBoxTextColourId,
        juce::Colours::white
    );

    slider.setColour(
        juce::Slider::textBoxBackgroundColourId,
        KSKSKSColours::background.withAlpha(0.45f)
    );

    slider.setColour(
        juce::Slider::textBoxOutlineColourId,
        juce::Colours::transparentBlack
    );

    slider.setColour(
        juce::Slider::textBoxHighlightColourId,
        KSKSKSColours::accent.withAlpha(0.25f)
    );

    addAndMakeVisible(slider);
}

void KSKSKSAudioProcessorEditor::setupLabel(juce::Label& label, const juce::String& text) {
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);

    label.setColour(
        juce::Label::textColourId,
        KSKSKSColours::muted
    );

    label.setFont(juce::Font(12.0f, juce::Font::bold));

    addAndMakeVisible(label);
}

std::unique_ptr<juce::ComponentTraverser>
KSKSKSAudioProcessorEditor::createKeyboardFocusTraverser(){
    std::vector<juce::Component*> fields {
        &modeBox,
        &divisionBox,
        findSliderTextBox(depthSlider),
        findSliderTextBox(curveSlider),
        findSliderTextBox(attackSlider),
        findSliderTextBox(releaseSlider),
        findSliderTextBox(thresholdSlider)
    };

    fields.erase(
        std::remove(fields.begin(), fields.end(), nullptr),
        fields.end()
    );

    return std::make_unique<ValueFieldFocusTraverser>(std::move(fields));
}

void KSKSKSAudioProcessorEditor::focusField(int index) {
    constexpr int fieldCount = 7;

    index = (index % fieldCount + fieldCount) % fieldCount;

    switch (index) {
        case 0:
            modeBox.grabKeyboardFocus();
            break;

        case 1:
            divisionBox.grabKeyboardFocus();
            break;

        default:
        {
            ValueSlider* sliders[] = {
                &depthSlider,
                &curveSlider,
                &attackSlider,
                &releaseSlider,
                &thresholdSlider
            };

            const int sliderIndex = index - 2;

            if (auto* label = dynamic_cast<RoundedValueLabel*>(
                findSliderTextBox(*sliders[sliderIndex])))
            {
                label->showEditor();
            }

            break;
        }
    }
}

bool KSKSKSAudioProcessorEditor::keyPressed(
    const juce::KeyPress& key,
    juce::Component* originatingComponent
) {
    if (key.getKeyCode() != juce::KeyPress::tabKey)
        return false;

    const bool backwards =
        key.getModifiers().isShiftDown();

    if (originatingComponent == &modeBox)
    {
        focusField(backwards ? 6 : 1);
        return true;
    }

    if (originatingComponent == &divisionBox)
    {
        focusField(backwards ? 0 : 2);
        return true;
    }

    return false;
}

void KSKSKSAudioProcessorEditor::paint(juce::Graphics& g) {
    using namespace KSKSKSColours;

    g.fillAll(background);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(27.0f, juce::Font::bold));

    g.drawText(
        "KSKSKS",
        24, 16, 230, 36,
        juce::Justification::centredLeft);

    g.setColour(muted);
    g.setFont(juce::Font(11.5f));

    g.drawText(
        "Kinda Simple Kick Start: Kick & Sync",
        26, 50, 430, 18,
        juce::Justification::centredLeft);

    const juce::String tagline = "by~ ATK :3";
    const juce::Font taglineFont(11.0f);

    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(taglineFont, tagline, 0.0f, 0.0f);

    const int textWidth = juce::roundToInt(glyphs.getBoundingBox(0, -1, true).getWidth());

    const int rightEdge = 736;

    const int badgeWidth = textWidth + 12;

    const auto badge = juce::Rectangle<int>(rightEdge - badgeWidth, 20, badgeWidth, 26);

    g.setColour(juce::Colour(43, 62, 48));
    g.fillRoundedRectangle(badge.toFloat(), 7.0f);

    g.setColour(accent);
    g.setFont(juce::Font(11.5f, juce::Font::bold));

    g.drawText("V0.2.7", badge, juce::Justification::centred);

    g.setColour(muted);
    g.setFont(taglineFont);

    g.drawText(
        tagline,
        badge.getX(), 50, badge.getWidth(), 18,
        juce::Justification::centred
    );

    g.setColour(panel);

    g.fillRoundedRectangle(
        juce::Rectangle<float>(20, 92, 720, 104), 12.0f);

    g.fillRoundedRectangle(
        juce::Rectangle<float>(20, 210, 720, 172), 12.0f);

    g.fillRoundedRectangle(
        juce::Rectangle<float>(20, 394, 720, 88), 12.0f);

    const bool isKickMode = modeBox.getSelectedId() == 2;

    if (isKickMode) {
        const bool hasSidechainSignal = displayedSidechainLevel > 0.001f;

        g.setColour(hasSidechainSignal ? accent : muted);

        g.fillEllipse(220.0f, 107.0f, 10.0f, 10.0f);

        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(11.0f, juce::Font::bold));

        g.drawText(
            hasSidechainSignal
                ? "SIDECHAIN: SIGNAL"
                : "SIDECHAIN: NO SIGNAL",
            235, 101, 155, 22,
            juce::Justification::centredLeft
        );

        g.setColour(muted);
        g.setFont(juce::Font(12.0f));

        g.drawText(
            "Select Kick in Wrapper",
            235, 120, 170, 20,
            juce::Justification::centredLeft
        );
    }

    const auto graph = juce::Rectangle<float>(44, 246, 672, 112);

    g.setColour(grid);

    for (int i = 1; i < 4; ++i) {
        const float y = graph.getY() + graph.getHeight() * i / 4.0f;

        g.drawLine(
            graph.getX(), y,
            graph.getRight(), y,
            1.0f
        );
    }

    for (int i = 1; i < 8; ++i) {
        const float x = graph.getX() + graph.getWidth() * i / 8.0f;

        g.drawLine(
            x, graph.getY(),
            x, graph.getBottom(),
            1.0f
        );
    }

    const float curve = audioProcessor.parameters.getRawParameterValue("CURVE")->load();

    juce::Path path;

    for (int i = 0; i <= 160; ++i) {
        const float t = i / 160.0f;
        const float recovery = std::pow(t, curve);

        const float x = graph.getX() + t * graph.getWidth();

        const float y = graph.getBottom() - recovery * graph.getHeight();

        if (i == 0) path.startNewSubPath(x, y);
        else        path.lineTo(x, y);
    }

    g.setColour(accent);

    g.strokePath(
        path,
        juce::PathStrokeType(
            2.5f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded
    ));

    g.setColour(muted);
    g.setFont(11.0f);

    g.drawText(
        "CURVE SHAPE PREVIEW",
        42, 218, 240, 20,
        juce::Justification::centredLeft);

    g.drawText(
        "LOW",
        44, 360, 60, 14,
        juce::Justification::centredLeft);

    g.drawText(
        "TIME",
        330, 360, 100, 14,
        juce::Justification::centred);

    g.drawText(
        "FULL LEVEL",
        620, 360, 96, 14,
        juce::Justification::centredRight);
}

void KSKSKSAudioProcessorEditor::resized() {
    modeLabel.setBounds(63, 101, 100, 20);
    modeBox.setBounds(38, 125, 150, 34);

    divisionLabel.setBounds(220, 101, 140, 20);
    divisionBox.setBounds(220, 125, 140, 34);

    sidechainInfoIcon.setBounds(220, 125, 10, 10);

    depthLabel.setBounds(370, 101, 100, 20);
    depthSlider.setBounds(370, 119, 100, 76);

    attackLabel.setBounds(490, 101, 100, 20);
    attackSlider.setBounds(490, 119, 100, 76);

    releaseLabel.setBounds(610, 101, 100, 20);
    releaseSlider.setBounds(610, 119, 100, 76);

    curveLabel.setBounds(40, 399, 100, 20);
    curveSlider.setBounds(35, 415, 110, 64);

    thresholdLabel.setBounds(160, 399, 140, 20);
    thresholdSlider.setBounds(160, 415, 140, 64);

    statusLabel.setBounds(320, 450, 390, 20);

    gainReductionTitleLabel.setBounds(635, 218, 34, 20);
    gainReductionValueLabel.setBounds(649, 218, 67, 20);
}