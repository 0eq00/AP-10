/*
  ==============================================================================
    PluginEditor.cpp
    AP-10 / GT913 Plugin Editor Implementation
    All comments are 100% English ASCII.
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::String AP10AudioProcessorEditor::toHex32(uint32_t val)
{
    return "0x" + juce::String::toHexString(static_cast<int>(val)).toUpperCase().paddedLeft('0', 8);
}

uint32_t AP10AudioProcessorEditor::parseHex32(const juce::String& text)
{
    juce::String clean = text.trim();
    if (clean.startsWithIgnoreCase("0x"))
        clean = clean.substring(2);
    return clean.getHexValue32();
}

AP10AudioProcessorEditor::AP10AudioProcessorEditor (AP10AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    addAndMakeVisible(loadRomButton);
    loadRomButton.onClick = [this]()
    {
        juce::File initialRomFile = AP10AudioProcessor::getDefaultRomFile();
        juce::File startLocation = initialRomFile.existsAsFile() ? initialRomFile : AP10AudioProcessor::getPluginDirectory();

        fileChooser = std::make_unique<juce::FileChooser>(
            "Select AP-10 ROM (mx23c8100mc-12ca17.lsi303 or ap10.lsi303)...",
            startLocation,
            "*.lsi303;*.bin;*.rom");

        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                audioProcessor.getEngine().clearRom();

                if (audioProcessor.getEngine().loadAndDecryptRom(file, false))
                {
                    audioProcessor.addDebugLog("ROM Loaded & Descrambled: " + file.getFileName()
                                                + " (" + juce::String(audioProcessor.getEngine().getRomSize() / 1024) + " KB)");
                    statusLabel.setText("ROM: " + file.getFileName() + " (Ready)", juce::dontSendNotification);
                }
                else
                {
                    audioProcessor.addDebugLog("Error: Could not decode ROM: " + file.getFullPathName());
                    statusLabel.setText("ROM load failed!", juce::dontSendNotification);
                }
            }
        });
    };

    addAndMakeVisible(statusLabel);
    statusLabel.setText("AP-10 (GT913 Sound Engine)", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    statusLabel.setFont(juce::FontOptions(15.0f, juce::Font::bold));

    addAndMakeVisible(voiceCountLabel);
    voiceCountLabel.setText("Voices: 0 / 24", juce::dontSendNotification);
    voiceCountLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    voiceCountLabel.setFont(juce::FontOptions(13.0f));

    addAndMakeVisible(toneLabel);
    toneLabel.setText("Tone:", juce::dontSendNotification);
    toneLabel.setColour(juce::Label::textColourId, juce::Colour(0xffd4d4d8));
    toneLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));

    addAndMakeVisible(toneSelector);
    toneSelector.addItem("1: Grand Piano", 1);
    toneSelector.addItem("2: E. Piano", 2);
    toneSelector.addItem("3: Harpsichord", 3);
    toneSelector.addItem("4: Pipe Organ", 4);
    toneSelector.addItem("5: Strings", 5);
    toneSelector.setSelectedId(audioProcessor.getEngine().current_preset + 1, juce::dontSendNotification);

    toneSelector.onChange = [this]()
    {
        int selectedIndex = toneSelector.getSelectedId() - 1;
        if (selectedIndex >= 0 && selectedIndex < 5)
        {
            audioProcessor.getEngine().setTone(selectedIndex);
            audioProcessor.setCurrentProgram(selectedIndex);

            const auto& preset = audioProcessor.getEngine().getCurrentPreset();
            decayEditor.setText(toHex32(preset.decay_rate), juce::dontSendNotification);
            releaseEditor.setText(toHex32(preset.release_rate), juce::dontSendNotification);
            sustainEditor.setText(toHex32(preset.sustain_level), juce::dontSendNotification);

            audioProcessor.addDebugLog("GUI Tone Changed: " + juce::String(preset.name));
            statusLabel.setText("Tone: " + juce::String(preset.name), juce::dontSendNotification);
        }
    };

    // Envelope controls setup
    addAndMakeVisible(envTitleLabel);
    envTitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff59e0b));
    envTitleLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));

    addAndMakeVisible(decayLabel);
    decayLabel.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(decayEditor);
    decayEditor.setMultiLine(false);
    decayEditor.setInputRestrictions(10, "0123456789abcdefABCDEFxX");

    addAndMakeVisible(releaseLabel);
    releaseLabel.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(releaseEditor);
    releaseEditor.setMultiLine(false);
    releaseEditor.setInputRestrictions(10, "0123456789abcdefABCDEFxX");

    addAndMakeVisible(sustainLabel);
    sustainLabel.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(sustainEditor);
    sustainEditor.setMultiLine(false);
    sustainEditor.setInputRestrictions(10, "0123456789abcdefABCDEFxX");

    const auto& initPreset = audioProcessor.getEngine().getCurrentPreset();
    decayEditor.setText(toHex32(initPreset.decay_rate), juce::dontSendNotification);
    releaseEditor.setText(toHex32(initPreset.release_rate), juce::dontSendNotification);
    sustainEditor.setText(toHex32(initPreset.sustain_level), juce::dontSendNotification);

    auto updateEngineEnvelope = [this]()
    {
        int idx = audioProcessor.getEngine().current_preset;
        uint32_t d = parseHex32(decayEditor.getText());
        uint32_t r = parseHex32(releaseEditor.getText());
        uint32_t s = parseHex32(sustainEditor.getText());
        audioProcessor.getEngine().setEnvelopeParams(idx, d, r, s);
    };

    decayEditor.onTextChange = updateEngineEnvelope;
    releaseEditor.onTextChange = updateEngineEnvelope;
    sustainEditor.onTextChange = updateEngineEnvelope;

    addAndMakeVisible(consoleOutput);
    consoleOutput.setMultiLine(true);
    consoleOutput.setReadOnly(true);
    consoleOutput.setScrollbarsShown(true);
    consoleOutput.setCaretVisible(false);
    consoleOutput.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff18181b));
    consoleOutput.setColour(juce::TextEditor::textColourId, juce::Colour(0xff4ade80));

    setSize(680, 480);
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

    int currentToneId = audioProcessor.getEngine().current_preset + 1;
    if (toneSelector.getSelectedId() != currentToneId)
    {
        toneSelector.setSelectedId(currentToneId, juce::dontSendNotification);
        const auto& preset = audioProcessor.getEngine().getCurrentPreset();
        decayEditor.setText(toHex32(preset.decay_rate), juce::dontSendNotification);
        releaseEditor.setText(toHex32(preset.release_rate), juce::dontSendNotification);
        sustainEditor.setText(toHex32(preset.sustain_level), juce::dontSendNotification);
    }

    repaint();
}

void AP10AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff09090b));

    g.setColour(juce::Colour(0xff27272a));
    g.fillRect(0, 0, getWidth(), 50);

    g.setColour(juce::Colours::gold);
    g.drawRect(0, 0, getWidth(), 50, 1);

    // Draw envelope tuner bar background
    g.setColour(juce::Colour(0xff18181b));
    g.fillRect(0, 50, getWidth(), 35);
    g.setColour(juce::Colour(0xff27272a));
    g.drawRect(0, 50, getWidth(), 35, 1);

    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("GT913 24-VOICE POLYPHONY MATRIX:", 15, 92, 300, 18, juce::Justification::left);

    int startX = 15;
    int startY = 114;
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
            g.setColour(juce::Colours::cyan);
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
    statusLabel.setBounds(15, 5, 230, 24);
    voiceCountLabel.setBounds(15, 26, 180, 20);

    toneLabel.setBounds(getWidth() - 410, 10, 45, 30);
    toneSelector.setBounds(getWidth() - 365, 10, 195, 30);
    loadRomButton.setBounds(getWidth() - 160, 10, 145, 30);

    // Envelope tuner bar layout (y: 50..85)
    envTitleLabel.setBounds(15, 57, 130, 20);
    decayLabel.setBounds(150, 57, 40, 20);
    decayEditor.setBounds(190, 54, 85, 22);
    releaseLabel.setBounds(285, 57, 50, 20);
    releaseEditor.setBounds(335, 54, 85, 22);
    sustainLabel.setBounds(430, 57, 50, 20);
    sustainEditor.setBounds(480, 54, 85, 22);

    consoleOutput.setBounds(15, 160, getWidth() - 30, getHeight() - 175);
}
