/*
  ==============================================================================
    PluginEditor.cpp
    AP-10 / GT913 Plugin Editor Implementation
    All comments are 100% English ASCII.
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

AP10AudioProcessorEditor::AP10AudioProcessorEditor (AP10AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      keyboardComponent (p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    addAndMakeVisible(statusLabel);
    statusLabel.setText("AP-10 (GT913 Sound Engine)", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    statusLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));

    // 1. voiceCountLabel: Font size matched to 11.0f (placed next to matrix title)
    addAndMakeVisible(voiceCountLabel);
    voiceCountLabel.setText("Voices: 0 / 24", juce::dontSendNotification);
    voiceCountLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4ade80));
    voiceCountLabel.setFont(juce::FontOptions(11.0f));

    // 2. 5 Tone Preset Toggle Buttons (replacing ComboBox)
    const juce::String buttonNames[5] = { "Piano", "E. Piano", "Harpsichord", "Pipe Organ", "Strings" };
    for (int i = 0; i < 5; ++i)
    {
        toneButtons[i].setButtonText(buttonNames[i]);
        addAndMakeVisible(toneButtons[i]);
        toneButtons[i].setRadioGroupId(1001);
        toneButtons[i].setClickingTogglesState(true);
        toneButtons[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xff18181b));
        toneButtons[i].setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff27272a));
        toneButtons[i].setColour(juce::TextButton::textColourOffId, juce::Colour(0xffa1a1aa));
        toneButtons[i].setColour(juce::TextButton::textColourOnId, juce::Colour(0xff4ade80));

        toneButtons[i].onClick = [this, i]()
        {
            if (toneButtons[i].getToggleState())
            {
                audioProcessor.getEngine().setTone(i);
                audioProcessor.setCurrentProgram(i);

                const auto& preset = audioProcessor.getEngine().getCurrentPreset();

                audioProcessor.addDebugLog("GUI Tone Changed: " + juce::String(preset.name));
                statusLabel.setText("Tone: " + juce::String(preset.name), juce::dontSendNotification);
            }
        };
    }

    int initTone = audioProcessor.getEngine().current_preset;
    if (initTone >= 0 && initTone < 5)
        toneButtons[initTone].setToggleState(true, juce::dontSendNotification);

    // Release Rate Multiplier Rotary Knob (positioned directly beneath Strings button)
    addAndMakeVisible(releaseKnob);
    releaseKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    releaseKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 54, 16);
    releaseKnob.setRange(0.1, 5.0, 0.05);
    releaseKnob.setValue(audioProcessor.getEngine().getReleaseMultiplier(), juce::dontSendNotification);
    releaseKnob.setTextValueSuffix("x");
    releaseKnob.setDoubleClickReturnValue(true, 1.0);
    releaseKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xfffbbf24));
    releaseKnob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff27272a));
    releaseKnob.setColour(juce::Slider::thumbColourId, juce::Colour(0xff4ade80));
    releaseKnob.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe4e4e7));
    releaseKnob.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff18181b));
    releaseKnob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff27272a));
    releaseKnob.onValueChange = [this]()
    {
        float mult = static_cast<float>(releaseKnob.getValue());
        audioProcessor.getEngine().setReleaseMultiplier(mult);
    };

    addAndMakeVisible(releaseLabel);
    releaseLabel.setText("Release Rate", juce::dontSendNotification);
    releaseLabel.setFont(juce::FontOptions(10.0f));
    releaseLabel.setJustificationType(juce::Justification::centred);
    releaseLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa1a1aa));

    // 88-Key Interactive Piano Keyboard (playable with mouse, velocity varies with click Y position)
    addAndMakeVisible(keyboardComponent);
    keyboardComponent.setAvailableRange(21, 108); // 88 keys (MIDI 21 A0 to 108 C8)
    keyboardComponent.setLowestVisibleKey(21);
    keyboardComponent.setVelocity(0.8f, true); // true = velocity varies based on vertical click position
    keyboardComponent.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xfff4f4f5));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff18181b));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xff27272a));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour(0x204ade80));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0x804ade80)); // emerald green on key press
    keyboardComponent.setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colour(0x40000000));

    // 6 & 7. Collapsible Console Output (hidden by default, expands downwards on click)
    addAndMakeVisible(consoleOutput);
    consoleOutput.setMultiLine(true);
    consoleOutput.setReadOnly(true);
    consoleOutput.setScrollbarsShown(true);
    consoleOutput.setCaretVisible(false);
    consoleOutput.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff18181b));
    consoleOutput.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff27272a));
    consoleOutput.setColour(juce::TextEditor::textColourId, juce::Colour(0xff4ade80));
    consoleOutput.setVisible(false);

    // Small toggle button at the bottom-right corner
    addAndMakeVisible(toggleConsoleButton);
    toggleConsoleButton.setButtonText("Log " + juce::String::fromUTF8("▼"));
    toggleConsoleButton.setTooltip("Show / Hide Debug Console");
    toggleConsoleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff18181b));
    toggleConsoleButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff27272a));
    toggleConsoleButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffa1a1aa));
    toggleConsoleButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xff4ade80));

    toggleConsoleButton.onClick = [this]()
    {
        isConsoleExpanded = !isConsoleExpanded;
        if (isConsoleExpanded)
        {
            toggleConsoleButton.setButtonText("Log " + juce::String::fromUTF8("▲"));
            consoleOutput.setVisible(true);
            setSize(680, 380);
        }
        else
        {
            toggleConsoleButton.setButtonText("Log " + juce::String::fromUTF8("▼"));
            consoleOutput.setVisible(false);
            setSize(680, 296);
        }
    };

    // Plugin window size with collapsed console by default (262px)
    setSize(680, 296);
    startTimerHz(25);
}

AP10AudioProcessorEditor::~AP10AudioProcessorEditor()
{
    stopTimer();
}

void AP10AudioProcessorEditor::timerCallback()
{
    juce::StringArray newLogs = audioProcessor.exchangeLogBuffer();
    for (auto& log : newLogs)
    {
        consoleOutput.insertTextAtCaret(log + juce::newLine);
    }

    int active = audioProcessor.getActiveVoiceCount();
    voiceCountLabel.setText("Voices: " + juce::String(active) + " / 24", juce::dontSendNotification);

    // Sync toggle button states if preset changed externally
    int currentToneId = audioProcessor.getEngine().current_preset;
    if (currentToneId >= 0 && currentToneId < 5)
    {
        if (!toneButtons[currentToneId].getToggleState())
        {
            toneButtons[currentToneId].setToggleState(true, juce::dontSendNotification);
        }
    }

    // Sync release knob if changed externally (e.g. from DAW preset / state restore)
    if (!releaseKnob.isMouseButtonDown())
    {
        float currentMult = audioProcessor.getEngine().getReleaseMultiplier();
        if (std::abs(releaseKnob.getValue() - currentMult) > 0.01)
        {
            releaseKnob.setValue(currentMult, juce::dontSendNotification);
        }
    }

    repaint();
}

void AP10AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff09090b));

    // Top header bar background & border
    g.setColour(juce::Colour(0xff27272a));
    g.fillRect(4, 4, getWidth()-8, 118);
    g.setColour(juce::Colours::gold);
    g.drawRect(4, 4, getWidth()-8, 118, 1);

    // 4 & 5. Dividing line between ENVELOPE PARAMS and POLYPHONY MATRIX in console border grey
    g.setColour(juce::Colour(0xff27272a));
    g.drawHorizontalLine(120, 0.0f, static_cast<float>(getWidth()));

    // 1. Section title for 24-Voice Polyphony Matrix
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("GT913 24-VOICE POLYPHONY MATRIX", 15, 126, 220, 18, juce::Justification::left);

    int startX = 15;
    int startY = 146;
    int ledW = 20;
    int ledH = 14;
    int gap = 4;

    auto& engine = audioProcessor.getEngine();
    for (int i = 0; i < 24; ++i)
    {
        int x = startX + (i % 12) * (ledW + gap);
        int y = startY + (i / 12) * (ledH + gap + 4);

        bool active = engine.voices[i].m_enable;
        if (active)
        {
            g.setColour(juce::Colour(0xff4ade80));
            g.fillRoundedRectangle((float)x, (float)y, (float)ledW, (float)ledH, 3.0f);
            g.setColour(juce::Colours::black);
            g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
            g.drawText(juce::String(engine.voices[i].m_midi_note), x, y, ledW, ledH, juce::Justification::centred);
        }
        else
        {
            g.setColour(juce::Colour(0xff27272a));
            g.drawRoundedRectangle((float)x, (float)y, (float)ledW, (float)ledH, 3.0f, 1.0f);
            g.setColour(juce::Colour(0xff71717a));
            g.setFont(juce::FontOptions(8.0f));
            g.drawText(juce::String(i + 1), x, y, ledW, ledH, juce::Justification::centred);
        }
    }
}

void AP10AudioProcessorEditor::resized()
{
    statusLabel.setBounds(15, 12, 185, 26);

    // 2. 5 Tone toggle buttons on top header bar
    int btnW = 88;
    int btnH = 28;
    int btnGap = 4;
    int btnStartX = getWidth() - 15 - (5 * btnW + 4 * btnGap); // aligned to right
    for (int i = 0; i < 5; ++i)
    {
        toneButtons[i].setBounds(btnStartX + i * (btnW + btnGap), 11, btnW, btnH);
    }

    // Release Rate Knob positioned directly beneath the 5th (Strings) button
    int stringsBtnX = btnStartX + 4 * (btnW + btnGap);
    int knobW = 56;
    int knobX = stringsBtnX + (btnW - knobW) / 2;
    releaseLabel.setBounds(stringsBtnX, 42, btnW, 14);
    releaseKnob.setBounds(knobX, 56, knobW, 58);

    // 1. voiceCountLabel placed right next to "GT913 24-VOICE POLYPHONY MATRIX"
    voiceCountLabel.setBounds(240, 126, 120, 18);

    // 88-Key Interactive Piano Keyboard (placed directly above the console, with 10px gap below polyphony LEDs)
    keyboardComponent.setBounds(15, 192, getWidth() - 30, 64);
    keyboardComponent.setKeyWidth(static_cast<float>(getWidth() - 30) / 52.0f);

    // 7. Collapsible console output & bottom-right toggle button
    int toggleW = 54;
    int toggleH = 18;

    if (isConsoleExpanded)
    {
        // 6. Console Output (shortened to ~5 lines, 80px height)
        consoleOutput.setBounds(15, 264, getWidth() - 30, 80);
        toggleConsoleButton.setBounds(getWidth() - 15 - toggleW, 349, toggleW, toggleH);
    }
    else
    {
        consoleOutput.setBounds(0, 0, 0, 0);
        toggleConsoleButton.setBounds(getWidth() - 15 - toggleW, 263, toggleW, toggleH);
    }
}
