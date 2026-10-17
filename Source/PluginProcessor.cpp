/*
  ==============================================================================
    PluginProcessor.cpp
    AP-10 / GT913 Synthesizer Plugin Processor Implementation
    All comments are 100% English ASCII.
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::File AP10AudioProcessor::getPluginDirectory()
{
    juce::File currentModule = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    juce::File dir = currentModule.isDirectory() ? currentModule : currentModule.getParentDirectory();

    juce::File search = dir;
    for (int i = 0; i < 4; ++i)
    {
        if (search.getChildFile("ap10.lsi303").existsAsFile())
            return search;
        if (search.getFileName().endsWithIgnoreCase(".vst3"))
        {
            if (search.getChildFile("ap10.lsi303").existsAsFile())
                return search;
            if (search.getParentDirectory().getChildFile("ap10.lsi303").existsAsFile())
                return search.getParentDirectory();
            return search;
        }
        if (search.getParentDirectory() == search)
            break;
        search = search.getParentDirectory();
    }

    return dir;
}

juce::File AP10AudioProcessor::getDefaultRomFile()
{
    juce::File pluginDir = getPluginDirectory();
    juce::File targetRom = pluginDir.getChildFile("ap10.lsi303");
    if (targetRom.existsAsFile())
        return targetRom;

    if (pluginDir.getParentDirectory().getChildFile("ap10.lsi303").existsAsFile())
        return pluginDir.getParentDirectory().getChildFile("ap10.lsi303");

    return targetRom;
}

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
    initializeSostenuto();
    juce::File romFile = getDefaultRomFile();
    if (romFile.existsAsFile() && engine.loadAndDecryptRom(romFile, false))
    {
        addDebugLog("GT913: Loaded ROM : " + romFile.getFullPathName()
                    + " (" + juce::String(engine.getRomSize() / 1024) + " KB)!");
    }
}

void AP10AudioProcessor::initializeSostenuto()
{
    isSostenutoPedalOn = false;
    for (int i = 0; i < 128; ++i)
    {
        keyOn[i] = false;
        sosOn[i] = false;
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
    const int limit = engine.getMaxVoices();
    for (int i = 0; i < limit; ++i)
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

    // Band-limited conversion from the engine rate (about 114.5 kHz) to the host rate
    resampler.prepare(GT913Engine::HARDWARE_SAMPLE_RATE, sampleRate);

    initializeSostenuto();
    addDebugLog("GT913: prepareToPlay at host rate " + juce::String(sampleRate) + " Hz");

    if (!engine.isRomLoaded())
    {
        juce::File romFile = getDefaultRomFile();
        if (romFile.existsAsFile() && engine.loadAndDecryptRom(romFile, false))
        {
            addDebugLog("GT913: Loaded ROM : " + romFile.getFullPathName()
                        + " (" + juce::String(engine.getRomSize() / 1024) + " KB)!");
        }
    }

    if (engine.isRomLoaded())
    {
        addDebugLog("GT913: ROM active (" + juce::String(engine.getRomSize() / 1024) + " KB). Ready.");
    }
    else
    {
        addDebugLog("GT913: ROM not loaded yet. Please place ap10.lsi303 in plugin folder or click 'Load ROM'.");
    }
}

void AP10AudioProcessor::releaseResources()
{
    engine.reset();
    initializeSostenuto();
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

    // Process on-screen interactive keyboard state & inject MIDI messages
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();

        // MIDI CC#66 Sostenuto Pedal logic based on SostenutoPedal
        if (msg.isSostenutoPedalOn())
        {
            isSostenutoPedalOn = true;
            for (int i = 0; i < 128; ++i)
            {
                sosOn[i] = keyOn[i];
            }
            addDebugLog("CC#66 Sostenuto On");
            continue;
        }
        else if (msg.isSostenutoPedalOff())
        {
            isSostenutoPedalOn = false;
            for (int i = 0; i < 128; ++i)
            {
                if (sosOn[i] && !keyOn[i])
                {
                    engine.noteOff(i);
                    addDebugLog("NoteOff (Sost): " + juce::String(i));
                }
                sosOn[i] = false;
            }
            addDebugLog("CC#66 Sostenuto Off");
            continue;
        }

        if (msg.isNoteOn())
        {
            int note = msg.getNoteNumber();
            int vel  = msg.getVelocity();
            if (note >= 0 && note < 128)
                keyOn[note] = true;
            engine.noteOn(note, vel);
            addDebugLog("NoteOn: " + juce::String(note) + " vel: " + juce::String(vel));
        }
        else if (msg.isNoteOff())
        {
            int note = msg.getNoteNumber();
            if (note >= 0 && note < 128)
            {
                keyOn[note] = false;
                if (sosOn[note])
                {
                    continue; // Key released but sustained by Sostenuto pedal
                }
            }
            engine.noteOff(note);
            addDebugLog("NoteOff: " + juce::String(note));
        }
        else if (msg.isController())
        {
            if (msg.getControllerNumber() == 64)
            {
                bool down = msg.getControllerValue() >= 64;
                engine.setDamperPedal(down);
            }
            else if (msg.getControllerNumber() == 66)
            {
                // Fallback CC#66 handling if not caught by isSostenutoPedalOn/Off
                bool down = msg.getControllerValue() >= 64;
                if (down && !isSostenutoPedalOn)
                {
                    isSostenutoPedalOn = true;
                    for (int i = 0; i < 128; ++i)
                        sosOn[i] = keyOn[i];
                    addDebugLog("CC#66 Sostenuto On");
                }
                else if (!down && isSostenutoPedalOn)
                {
                    isSostenutoPedalOn = false;
                    for (int i = 0; i < 128; ++i)
                    {
                        if (sosOn[i] && !keyOn[i])
                        {
                            engine.noteOff(i);
                            addDebugLog("NoteOff (Sost): " + juce::String(i));
                        }
                        sosOn[i] = false;
                    }
                    addDebugLog("CC#66 Sostenuto Off");
                }
            }
            else if (msg.getControllerNumber() == 67)
            {
                bool down = msg.getControllerValue() >= 64;
                engine.setSoftPedal(down);
            }
            else if (msg.getControllerNumber() == 123)
            {
                engine.reset();
                initializeSostenuto();
            }
        }
        else if (msg.isProgramChange())
        {
            engine.setTone(msg.getProgramChangeNumber());
        }
    }

    auto* channelDataLeft  = buffer.getWritePointer(0);
    auto* channelDataRight = buffer.getWritePointer(1);

    const float softMultiplier = engine.soft_pedal ? 0.7f : 1.0f;
    const float masterVol = 0.5f * softMultiplier;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        // Run the engine at its own rate (about 114.5 kHz) until the resampler
        // has enough source frames for the next output sample.
        while (resampler.needsInput())
        {
            int64_t stepLeft = 0;
            int64_t stepRight = 0;

            const int limit = engine.getMaxVoices();
            for (int v = 0; v < limit; ++v)
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

            resampler.push(static_cast<float>(stepLeft)  / 45000000000.0f,
                           static_cast<float>(stepRight) / 45000000000.0f);
        }

        float finalLeft  = 0.0f;
        float finalRight = 0.0f;
        resampler.pop(finalLeft, finalRight);

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

void AP10AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement xml ("AP10PLUGINSTATE");
    xml.setAttribute ("currentPreset", engine.current_preset);
    xml.setAttribute ("max_voices", engine.getMaxVoices());
    xml.setAttribute ("release_multiplier", static_cast<double>(engine.getReleaseMultiplier()));
    xml.setAttribute ("piano_high_release_multiplier", static_cast<double>(engine.getPianoHighReleaseMultiplier()));

    copyXmlToBinary (xml, destData);
}

void AP10AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName ("AP10PLUGINSTATE"))
    {
        int presetIdx = xmlState->getIntAttribute ("currentPreset", 0);
        engine.setTone (presetIdx);

        // Restore release_multiplier (default 1.0)
        double mult = xmlState->getDoubleAttribute ("release_multiplier",
                          xmlState->getDoubleAttribute ("releaseMultiplier", 1.0));
        engine.setReleaseMultiplier (static_cast<float>(mult));

        double highMult = xmlState->getDoubleAttribute ("piano_high_release_multiplier",
                              xmlState->getDoubleAttribute ("pianoHighReleaseMultiplier", 1.0));
        engine.setPianoHighReleaseMultiplier (static_cast<float>(highMult));

        int maxV = xmlState->getIntAttribute ("max_voices", 24);
        engine.setMaxVoices (maxV);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AP10AudioProcessor();
}
