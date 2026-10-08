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

    static juce::String toHex32(uint32_t val);
    static uint32_t parseHex32(const juce::String& text);

private:
    AP10AudioProcessor& audioProcessor;

    juce::TextButton loadRomButton { "Load ROM (ap10.lsi303)" };
    juce::Label toneLabel { {}, "Tone:" };
    juce::ComboBox toneSelector;
    juce::Label statusLabel;
    juce::Label voiceCountLabel;

    // Envelope parameter controls (decay_rate, release_rate, sustain_level)
    juce::Label envTitleLabel { {}, "ENVELOPE PARAMS (HEX):" };
    juce::Label decayLabel { {}, "Decay:" };
    juce::TextEditor decayEditor;
    juce::Label releaseLabel { {}, "Release:" };
    juce::TextEditor releaseEditor;
    juce::Label sustainLabel { {}, "Sustain:" };
    juce::TextEditor sustainEditor;

    juce::TextEditor consoleOutput;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AP10AudioProcessorEditor)
};
