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
    // 1. Two separate top-left labels: "AP-10" (larger font) & "GT913 Sound Engine" (smaller font)
    addAndMakeVisible(titleLabel);
    titleLabel.setText("AP-10", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(myColor::textWhite));
    titleLabel.setFont(juce::FontOptions(17.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(subtitleLabel);
    subtitleLabel.setText("GT913 Sound Engine", juce::dontSendNotification);
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa1a1aa));
    subtitleLabel.setFont(juce::FontOptions(10.5f, juce::Font::plain));
    subtitleLabel.setJustificationType(juce::Justification::centredLeft);

    // 1. voiceCountLabel: Font size matched to 11.0f (placed next to matrix title)
    addAndMakeVisible(voiceCountLabel);
    voiceCountLabel.setText("Voices: 0 / " + juce::String(audioProcessor.getMaxVoices()), juce::dontSendNotification);
    voiceCountLabel.setColour(juce::Label::textColourId, juce::Colour(myColor::activeTeal));
    voiceCountLabel.setFont(juce::FontOptions(11.0f));

    // Polyphony Mode Toggle Button (24-Voice default vs 88-Voice)
    addAndMakeVisible(voiceModeButton);
    voiceModeButton.setButtonText(audioProcessor.getMaxVoices() == 88 ? "88 Voices" : "24 Voices");
    voiceModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff18181b));
    voiceModeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(myColor::activeTeal));
    voiceModeButton.onClick = [this]()
    {
        int newLimit = (audioProcessor.getMaxVoices() == 24) ? 88 : 24;
        audioProcessor.setMaxVoices(newLimit);
        voiceModeButton.setButtonText(newLimit == 88 ? "88 Voices" : "24 Voices");
        voiceModeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(newLimit == 88 ? myColor::activePink : myColor::activeTeal));
        voiceCountLabel.setColour(juce::Label::textColourId, juce::Colour(newLimit == 88 ? myColor::activePink : myColor::activeTeal));
        voiceCountLabel.setText("Voices: " + juce::String(audioProcessor.getActiveVoiceCount()) + " / " + juce::String(newLimit), juce::dontSendNotification);
        repaint();
    };

    // 2. 5 Tone Preset Toggle Buttons (replacing ComboBox) with smaller font
    const juce::String buttonNames[5] = { "Piano", "E. Piano", "Harpsichord", "Pipe Organ", "Strings" };
    for (int i = 0; i < 5; ++i)
    {
        toneButtons[i].setLookAndFeel(&ringKnobLookAndFeel);
        toneButtons[i].setButtonText(buttonNames[i]);
        addAndMakeVisible(toneButtons[i]);
        toneButtons[i].setRadioGroupId(1001);
        toneButtons[i].setClickingTogglesState(true);
        toneButtons[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xff18181b));
        toneButtons[i].setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff27272a));
        toneButtons[i].setColour(juce::TextButton::textColourOffId, juce::Colour(0xffa1a1aa));
        toneButtons[i].setColour(juce::TextButton::textColourOnId, juce::Colour(myColor::activeTeal));

        toneButtons[i].onClick = [this, i]()
        {
            if (toneButtons[i].getToggleState())
            {
                audioProcessor.getEngine().setTone(i);
                audioProcessor.setCurrentProgram(i);

                const auto& preset = audioProcessor.getEngine().getCurrentPreset();

                audioProcessor.addDebugLog("GUI Tone Changed: " + juce::String(preset.name));
            }
        };
    }

    int initTone = audioProcessor.getEngine().current_preset;
    if (initTone >= 0 && initTone < 5)
        toneButtons[initTone].setToggleState(true, juce::dontSendNotification);

    // Release Rate Multiplier Rotary Knob (custom minimalist ring knob matching knob.png)
    addAndMakeVisible(releaseKnob);
    releaseKnob.setLookAndFeel(&ringKnobLookAndFeel);
    releaseKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    releaseKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 54, 16);
    releaseKnob.setRange(0.1, 5.0, 0.05);
    releaseKnob.setValue(audioProcessor.getEngine().getReleaseMultiplier(), juce::dontSendNotification);
    releaseKnob.setTextValueSuffix("x");
    releaseKnob.setDoubleClickReturnValue(true, 1.0);
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

    // Special Piano 92+ Release Rate Multiplier Knob (placed directly beneath Strings button)
    addAndMakeVisible(pianoHighReleaseKnob);
    pianoHighReleaseKnob.setLookAndFeel(&ringKnobLookAndFeel);
    pianoHighReleaseKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    pianoHighReleaseKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 54, 16);
    pianoHighReleaseKnob.setRange(0.1, 5.0, 0.05);
    pianoHighReleaseKnob.setValue(audioProcessor.getEngine().getPianoHighReleaseMultiplier(), juce::dontSendNotification);
    pianoHighReleaseKnob.setTextValueSuffix("x");
    pianoHighReleaseKnob.setDoubleClickReturnValue(true, 1.0);
    pianoHighReleaseKnob.onValueChange = [this]()
    {
        float mult = static_cast<float>(pianoHighReleaseKnob.getValue());
        audioProcessor.getEngine().setPianoHighReleaseMultiplier(mult);
    };

    addAndMakeVisible(pianoHighReleaseLabel);
    pianoHighReleaseLabel.setText("Piano 92+ Rel", juce::dontSendNotification);
    pianoHighReleaseLabel.setFont(juce::FontOptions(10.0f));
    pianoHighReleaseLabel.setJustificationType(juce::Justification::centred);
    pianoHighReleaseLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa1a1aa));


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
    consoleOutput.setColour(juce::TextEditor::textColourId, juce::Colour(myColor::activeTeal));
    consoleOutput.setVisible(false);

    // Small toggle button at the bottom-right corner
    addAndMakeVisible(toggleConsoleButton);
    toggleConsoleButton.setButtonText("Log " + juce::String::fromUTF8("▼"));
    toggleConsoleButton.setTooltip("Show / Hide Debug Console");
    toggleConsoleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff18181b));
    toggleConsoleButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff27272a));
    toggleConsoleButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffa1a1aa));
    toggleConsoleButton.setColour(juce::TextButton::textColourOnId, juce::Colour(myColor::activeTeal));

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
    releaseKnob.setLookAndFeel(nullptr);
    pianoHighReleaseKnob.setLookAndFeel(nullptr);
    for (int i = 0; i < 5; ++i)
        toneButtons[i].setLookAndFeel(nullptr);
}

void AP10AudioProcessorEditor::timerCallback()
{
    juce::StringArray newLogs = audioProcessor.exchangeLogBuffer();
    for (auto& log : newLogs)
    {
        consoleOutput.insertTextAtCaret(log + juce::newLine);
    }

    int active = audioProcessor.getActiveVoiceCount();
    int maxV = audioProcessor.getMaxVoices();
    voiceCountLabel.setText("Voices: " + juce::String(active) + " / " + juce::String(maxV), juce::dontSendNotification);
    juce::String expectedMode = (maxV == 88) ? "88 Voices" : "24 Voices";
    if (voiceModeButton.getButtonText() != expectedMode)
        voiceModeButton.setButtonText(expectedMode);

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

    if (!pianoHighReleaseKnob.isMouseButtonDown())
    {
        float currentHighMult = audioProcessor.getEngine().getPianoHighReleaseMultiplier();
        if (std::abs(pianoHighReleaseKnob.getValue() - currentHighMult) > 0.01)
        {
            pianoHighReleaseKnob.setValue(currentHighMult, juce::dontSendNotification);
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

    // 1. Section title for Polyphony Matrix
    auto& engine = audioProcessor.getEngine();
    const int maxV = engine.getMaxVoices();
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("GT913 " + juce::String(maxV) + "-VOICE POLYPHONY MATRIX", 15, 126, 220, 18, juce::Justification::left);

    int startX = 15;
    int startY = 146;

    if (maxV == 88)
    {
        // 88-Voice mode: 4 rows of 22 LEDs
        int ledW = 24;
        int ledH = 8;
        int gapX = 4;
        int gapY = 2;

        for (int i = 0; i < 88; ++i)
        {
            int col = i % 22;
            int row = i / 22;
            int x = startX + col * (ledW + gapX);
            int y = startY + row * (ledH + gapY);

            bool active = engine.voices[i].m_enable;
            if (active)
            {
                g.setColour(juce::Colour(myColor::activePink));
                g.fillRoundedRectangle((float)x, (float)y, (float)ledW, (float)ledH, 2.0f);
            }
            else
            {
                g.setColour(juce::Colour(0xff27272a));
                g.drawRoundedRectangle((float)x, (float)y, (float)ledW, (float)ledH, 2.0f, 1.0f);
            }
        }
    }
    else
    {
        // 24-Voice mode: 2 rows of 12 LEDs
        int ledW = 20;
        int ledH = 14;
        int gap = 4;

        for (int i = 0; i < 24; ++i)
        {
            int x = startX + (i % 12) * (ledW + gap);
            int y = startY + (i / 12) * (ledH + gap + 4);

            bool active = engine.voices[i].m_enable;
            if (active)
            {
                g.setColour(juce::Colour(myColor::activeTeal));
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
}

void AP10AudioProcessorEditor::resized()
{
    // 1. Two separate top-left labels: "AP-10" (larger font) & "GT913 Sound Engine" (smaller font)
    titleLabel.setBounds(15, 11, 56, 28);
    subtitleLabel.setBounds(72, 13, 132, 24);

    // 2. 5 Tone toggle buttons on top header bar
    int btnW = 88;
    int btnH = 28;
    int btnGap = 4;
    int btnStartX = getWidth() - 15 - (5 * btnW + 4 * btnGap); // aligned to right
    for (int i = 0; i < 5; ++i)
    {
        toneButtons[i].setBounds(btnStartX + i * (btnW + btnGap), 11, btnW, btnH);
    }

    // Release Knobs:
    // pianoHighReleaseKnob is placed at the current releaseKnob position (directly under Strings button)
    // releaseKnob is shifted to the left
    int stringsBtnX = btnStartX + 4 * (btnW + btnGap);
    int knobW = 56;
    int knobH = 58;
    int gapKnob = 8;

    // Special Piano 92+ knob directly beneath Strings:
    int specialKnobX = stringsBtnX + (btnW - knobW) / 2;
    pianoHighReleaseLabel.setBounds(specialKnobX - 4, 42, knobW + 8, 14);
    pianoHighReleaseKnob.setBounds(specialKnobX, 56, knobW, knobH);

    // General releaseKnob shifted to the left:
    int generalKnobX = specialKnobX - knobW - gapKnob;
    releaseLabel.setBounds(generalKnobX - 4, 42, knobW + 8, 14);
    releaseKnob.setBounds(generalKnobX, 56, knobW, knobH);

    // 1. voiceCountLabel & voiceModeButton placed right next to "GT913 POLYPHONY MATRIX"
    voiceCountLabel.setBounds(240, 126, 95, 18);
    voiceModeButton.setBounds(340, 126, 75, 18);

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
