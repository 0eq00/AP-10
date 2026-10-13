/*
  ==============================================================================
    PluginEditor.h
    AP-10 / GT913 Plugin Editor Header
    All comments are 100% English ASCII.
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class AP10AudioProcessorEditor : public juce::AudioProcessorEditor,
                                 public juce::Timer
{
public:
    AP10AudioProcessorEditor (AP10AudioProcessor&);
    ~AP10AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    AP10AudioProcessor& audioProcessor;

    juce::Label statusLabel;
    juce::Label voiceCountLabel;

    // 5 Tone Preset Toggle Buttons (replacing ComboBox)
    juce::TextButton toneButtons[5];

    // 88-Key Interactive Piano Keyboard (placed above the console)
    juce::MidiKeyboardComponent keyboardComponent;

    // Collapsible Console Output (hidden by default, toggled via bottom-right button)
    bool isConsoleExpanded { false };
    juce::TextButton toggleConsoleButton;
    juce::TextEditor consoleOutput;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AP10AudioProcessorEditor)
};
