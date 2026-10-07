/*
  ==============================================================================
    PluginEditor.h
    CASIO AP-10 / GT913 Plugin Editor Header
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

    juce::TextButton loadRomButton { "Load ROM (ap10.lsi303)" };
    juce::Label toneLabel { {}, "Tone:" };
    juce::ComboBox toneSelector;
    juce::Label statusLabel;
    juce::Label voiceCountLabel;
    juce::TextEditor consoleOutput;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AP10AudioProcessorEditor)
};
