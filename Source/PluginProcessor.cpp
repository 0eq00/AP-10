/*
  ==============================================================================
    PluginProcessor.cpp
    CASIO AP-10 / GT913 Synthesizer Plugin Processor
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

AP10AudioProcessor::AP10AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // Try auto-loading ROM if present on filesystem (default: use_mame_word_swap = false)
    juce::File romFile("C:\\TMP\\ap10.lsi303");
    if (romFile.existsAsFile() && engine.loadAndDecryptRom(romFile, false))
    {
        addDebugLog("GT913: ROM ap10.lsi303 loaded in constructor ("
                    + juce::String(engine.getRomSize() / 1024) + " KB)!");
    }
}

AP10AudioProcessor::~AP10AudioProcessor()
{
}

const juce::String AP10AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AP10AudioProcessor::acceptsMidi() const         { return true; }
bool AP10AudioProcessor::producesMidi() const        { return false; }
bool AP10AudioProcessor::isMidiEffect() const         { return false; }
double AP10AudioProcessor::getTailLengthSeconds() const { return 0.0; }
int AP10AudioProcessor::getNumPrograms()            { return 1; }
int AP10AudioProcessor::getCurrentProgram()         { return 0; }
void AP10AudioProcessor::setCurrentProgram (int)     {}
const juce::String AP10AudioProcessor::getProgramName (int) { return {}; }
void AP10AudioProcessor::changeProgramName (int, const juce::String&) {}

void AP10AudioProcessor::addDebugLog(const juce::String& message)
{
    const juce::ScopedLock sl(logLock);
    logBuffer.add(message);
}

juce::StringArray AP10AudioProcessor::exchangeLogBuffer()
{
    const juce::ScopedLock sl(logLock);
    juce::StringArray temp = logBuffer;
    logBuffer.clear();
    return temp;
}

int AP10AudioProcessor::getActiveVoiceCount() const
{
    int count = 0;
    for (int i = 0; i < GT913Engine::MAX_VOICES; ++i)
    {
        if (engine.voices[i].m_enable)
            count++;
    }
    return count;
}

void AP10AudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    hostSampleRate = sampleRate;
    srcRatioCounter = 0.0;
    lpfLeft1 = lpfLeft2 = lpfRight1 = lpfRight2 = 0.0f;

    addDebugLog("GT913: prepareToPlay at host rate " + juce::String(sampleRate) + " Hz");

    // Only load from file if not already loaded
    if (!engine.isRomLoaded())
    {
        juce::File romFile("C:\\TMP\\ap10.lsi303");
        if (romFile.existsAsFile() && engine.loadAndDecryptRom(romFile, false))
        {
            addDebugLog("GT913: ROM loaded in prepareToPlay ("
                        + juce::String(engine.getRomSize() / 1024) + " KB)!");
        }
    }

    if (engine.isRomLoaded())
    {
        addDebugLog("GT913: ROM active (" + juce::String(engine.getRomSize() / 1024) + " KB). Ready.");
    }
    else
    {
        addDebugLog("GT913: ROM not loaded yet. Please click 'Load ROM' button in GUI.");
    }
}

void AP10AudioProcessor::releaseResources()
{
    engine.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AP10AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}
#endif

void AP10AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // 1. Process MIDI Events
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();

        if (msg.isNoteOn())
        {
            int note = msg.getNoteNumber();
            int vel  = msg.getVelocity();
            engine.noteOn(note, vel);
            addDebugLog("NoteOn: " + juce::String(note) + " vel: " + juce::String(vel));
        }
        else if (msg.isNoteOff())
        {
            int note = msg.getNoteNumber();
            engine.noteOff(note);
            addDebugLog("NoteOff: " + juce::String(note));
        }
        else if (msg.isController())
        {
            if (msg.getControllerNumber() == 64) // Sustain Pedal
            {
                bool down = msg.getControllerValue() >= 64;
                engine.setDamperPedal(down);
            }
            else if (msg.getControllerNumber() == 67) // Soft Pedal
            {
                bool down = msg.getControllerValue() >= 64;
                engine.setSoftPedal(down);
            }
            else if (msg.getControllerNumber() == 123) // All Notes Off
            {
                engine.reset();
            }
        }
        else if (msg.isProgramChange())
        {
            engine.setTone(msg.getProgramChangeNumber());
        }
    }

    // 2. Synthesize audio with downsampling from GT913 rate to Host Rate
    auto* channelDataLeft  = buffer.getWritePointer(0);
    auto* channelDataRight = buffer.getWritePointer(1);

    const double srcSampleRate = GT913Engine::HARDWARE_SAMPLE_RATE;
    const double dstSampleRate = hostSampleRate;
    const float softMultiplier = engine.soft_pedal ? 0.7f : 1.0f;
    const float masterVol = 0.5f * softMultiplier;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        srcRatioCounter += (srcSampleRate / dstSampleRate);

        int64_t accumulatedLeft  = 0;
        int64_t accumulatedRight = 0;
        int subSamplesMixed = 0;

        while (srcRatioCounter >= 1.0)
        {
            int64_t stepLeft = 0;
            int64_t stepRight = 0;

            // Mix all 24 voices
            for (int v = 0; v < GT913Engine::MAX_VOICES; ++v)
            {
                auto& voice = engine.voices[v];
                if (voice.m_enable)
                {
                    engine.update_envelope(voice);
                    if (voice.m_enable)
                    {
                        engine.mix_sample(voice, stepLeft, stepRight);
                    }
                }
            }

            accumulatedLeft  += stepLeft;
            accumulatedRight += stepRight;
            subSamplesMixed++;

            srcRatioCounter -= 1.0;
        }

        float finalLeft  = 0.0f;
        float finalRight = 0.0f;

        if (subSamplesMixed > 0)
        {
            float rawL = static_cast<float>(accumulatedLeft / subSamplesMixed)  / 45000000000.0f;
            float rawR = static_cast<float>(accumulatedRight / subSamplesMixed) / 45000000000.0f;

            // 2-pole cascaded reconstruction low-pass filter (~12kHz at 114.5kHz internal rate)
            // Emulates hardware Casio AP-10 analog filter and eliminates ADPCM switching & downsampling noise
            lpfLeft1 += lpfAlpha * (rawL - lpfLeft1);
            lpfLeft2 += lpfAlpha * (lpfLeft1 - lpfLeft2);
            lpfRight1 += lpfAlpha * (rawR - lpfRight1);
            lpfRight2 += lpfAlpha * (lpfRight1 - lpfRight2);

            finalLeft  = lpfLeft2;
            finalRight = lpfRight2;
        }
        else if (i > 0)
        {
            finalLeft  = channelDataLeft[i - 1];
            finalRight = channelDataRight[i - 1];
        }

        channelDataLeft[i]  = juce::jlimit(-1.0f, 1.0f, finalLeft * masterVol);
        channelDataRight[i] = juce::jlimit(-1.0f, 1.0f, finalRight * masterVol);
    }
}

bool AP10AudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AP10AudioProcessor::createEditor()
{
    return new AP10AudioProcessorEditor (*this);
}

void AP10AudioProcessor::getStateInformation (juce::MemoryBlock&) {}
void AP10AudioProcessor::setStateInformation (const void*, int) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AP10AudioProcessor();
}
