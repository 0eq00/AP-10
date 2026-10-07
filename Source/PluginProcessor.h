/*
  ==============================================================================
    PluginProcessor.h
    CASIO AP-10 / GT913 Synthesizer Plugin Processor
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GT913Engine.h"

class AP10AudioProcessor : public juce::AudioProcessor
{
public:
    AP10AudioProcessor();
    ~AP10AudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Logging & Diagnostics
    void addDebugLog(const juce::String& message);
    juce::StringArray exchangeLogBuffer();

    // Access to engine for GUI
    GT913Engine& getEngine() { return engine; }
    int getActiveVoiceCount() const;

private:
    GT913Engine engine;

    double srcRatioCounter = 0.0;
    double hostSampleRate = 44100.0;

    // 2-pole low-pass reconstruction filter (~12kHz at 114.5kHz internal clock)
    // Eliminates ultrasonic ADPCM switching noise and downsampling aliasing
    float lpfLeft1 = 0.0f;
    float lpfLeft2 = 0.0f;
    float lpfRight1 = 0.0f;
    float lpfRight2 = 0.0f;
    const float lpfAlpha = 0.45f;

    juce::CriticalSection logLock;
    juce::StringArray logBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AP10AudioProcessor)
};
