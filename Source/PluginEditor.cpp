/*
  ==============================================================================
    PluginEditor.cpp
    CASIO AP-10 / GT913 Plugin Editor Implementation
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

AP10AudioProcessorEditor::AP10AudioProcessorEditor (AP10AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    addAndMakeVisible(loadRomButton);
    loadRomButton.onClick = [this]()
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select Casio AP-10 ROM (mx23c8100mc-12ca17.lsi303 or ap10.lsi303)...",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.lsi303;*.bin;*.rom");

        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                // Explicitly clear previously loaded ROM buffer first to prevent append bug (2048 KB issue)
                audioProcessor.getEngine().clearRom();

                // Directly load into the processor's own engine (defaults to use_mame_word_swap = false)
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
    statusLabel.setText("Casio Celviano AP-10 (GT913 Sound Engine)", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    statusLabel.setFont(juce::FontOptions(15.0f, juce::Font::bold));

    addAndMakeVisible(voiceCountLabel);
    voiceCountLabel.setText("Voices: 0 / 24", juce::dontSendNotification);
    voiceCountLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    voiceCountLabel.setFont(juce::FontOptions(13.0f));

    // Tone / Preset Selector UI
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
            audioProcessor.addDebugLog("GUI Tone Changed: " + juce::String(audioProcessor.getEngine().presets[selectedIndex].name));
            statusLabel.setText("Tone: " + juce::String(audioProcessor.getEngine().presets[selectedIndex].name), juce::dontSendNotification);
        }
    };

    addAndMakeVisible(consoleOutput);
    consoleOutput.setMultiLine(true);
    consoleOutput.setReadOnly(true);
    consoleOutput.setScrollbarsShown(true);
    consoleOutput.setCaretVisible(false);
    consoleOutput.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff18181b));
    consoleOutput.setColour(juce::TextEditor::textColourId, juce::Colour(0xff4ade80));

    setSize(680, 450);
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

    // Sync GUI tone selector if changed via MIDI Program Change
    int currentToneId = audioProcessor.getEngine().current_preset + 1;
    if (toneSelector.getSelectedId() != currentToneId)
    {
        toneSelector.setSelectedId(currentToneId, juce::dontSendNotification);
    }

    repaint();
}

void AP10AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff09090b));

    // Draw Top Header
    g.setColour(juce::Colour(0xff27272a));
    g.fillRect(0, 0, getWidth(), 50);

    g.setColour(juce::Colours::gold);
    g.drawRect(0, 0, getWidth(), 50, 1);

    // Draw 24 Voice Indicator LEDs
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("GT913 24-VOICE POLYPHONY MATRIX:", 15, 60, 300, 18, juce::Justification::left);

    int startX = 15;
    int startY = 82;
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

    consoleOutput.setBounds(15, 130, getWidth() - 30, getHeight() - 145);
}
