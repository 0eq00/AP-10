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

class MinimalRingKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MinimalRingKnobLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffe4e4e7));
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff121920));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff22303c));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& /*slider*/) override
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto lineW = 3.2f;
        auto arcRadius = radius - lineW * 0.5f - 1.0f;
        auto center = bounds.getCentre();

        // 1. Dark circular track (complete 360 circle matching knob.png)
        g.setColour (juce::Colour (0xff22303c));
        g.drawEllipse (center.x - arcRadius, center.y - arcRadius, arcRadius * 2.0f, arcRadius * 2.0f, lineW);

        // 2. Active arc in vibrant teal / seafoam (knob.png: #38bfa0)
        auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        if (toAngle > rotaryStartAngle + 0.01f)
        {
            juce::Path activeArc;
            activeArc.addCentredArc (center.x, center.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, toAngle, true);
            g.setColour (juce::Colour (0xff38bfa0));
            g.strokePath (activeArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // 3. Crisp white circular dot indicator at current value position (knob.png)
        auto thumbAngle = toAngle - juce::MathConstants<float>::halfPi;
        auto thumbX = center.x + arcRadius * std::cos (thumbAngle);
        auto thumbY = center.y + arcRadius * std::sin (thumbAngle);
        auto thumbRadius = 2.8f;
        g.setColour (juce::Colours::white);
        g.fillEllipse (thumbX - thumbRadius, thumbY - thumbRadius, thumbRadius * 2.0f, thumbRadius * 2.0f);
    }
};

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
    MinimalRingKnobLookAndFeel ringKnobLookAndFeel;

    juce::Label statusLabel;
    juce::Label voiceCountLabel;
    // Polyphony Mode Switch Button (24-Voice default vs 88-Voice)
    juce::TextButton voiceModeButton;

    // 5 Tone Preset Toggle Buttons (replacing ComboBox)
    juce::TextButton toneButtons[5];

    // Release Rate Multiplier Rotary Knob & Direct Value Editor (positioned beneath Strings button)
    juce::Slider releaseKnob;
    juce::Label  releaseLabel;
    // Special Release Rate Knob for Grand Piano >= 92 (placed at Strings position)
    juce::Slider pianoHighReleaseKnob;
    juce::Label  pianoHighReleaseLabel;

    // 88-Key Interactive Piano Keyboard (placed above the console)
    juce::MidiKeyboardComponent keyboardComponent;

    // Collapsible Console Output (hidden by default, toggled via bottom-right button)
    bool isConsoleExpanded { false };
    juce::TextButton toggleConsoleButton;
    juce::TextEditor consoleOutput;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AP10AudioProcessorEditor)
};
