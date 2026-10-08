/*
  ==============================================================================
    GT913Engine.h
    GT913 Sound Engine & 24-Voice ADPCM Processor for JUCE
    Ported & adapted from MAME gt913.cpp & ctk551.cpp (AP-10)
    All comments are 100% English ASCII.
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>

class GT913Engine
{
public:
    static constexpr int MAX_VOICES = 24;
    static constexpr double HARDWARE_SAMPLE_RATE = 114483.0; // ~114.5 kHz internal rate

    // Look-up tables from gt913.cpp
    static const uint8_t exp_2_to_3[4];
    static const int8_t sample_7_to_8[128];
    static const uint16_t volume_ramp[17];

    struct Voice
    {
        bool     m_enable = false;
        int      m_midi_note = 60;
        int      m_velocity = 100;
        uint32_t m_addr_start = 0x06BB3A;
        uint32_t m_addr_end = 0x07585C;
        uint32_t m_addr_loop = 0x072345;
        uint32_t m_addr_current = 0;
        uint32_t m_addr_frac = 0;
        uint32_t m_pitch = 0x00649062;

        int32_t  m_sample = 0;
        int32_t  m_sample_next = 0;
        uint8_t  m_exp = 0;

        uint32_t m_volume_current = 0;
        uint32_t m_volume_target = 0;
        uint32_t m_volume_rate = 0;
        uint16_t m_gain = 0x0F;
        uint8_t  m_balance[2] = { 7, 7 };

        bool     m_is_releasing = false;
        int      m_env_stage = 0; // 0: idle, 1: attack, 2: decay, 3: sustain, 4: release
        uint32_t m_decay_rate = 0x00001000;
        uint32_t m_release_rate = 0x00080000;
        uint32_t m_sustain_level = 0;
    };

    Voice voices[MAX_VOICES];

    // ROM buffer stored directly inside the engine instance
    juce::MemoryBlock rom_data;
    bool rom_loaded = false;

    bool isRomLoaded() const { return rom_loaded; }
    size_t getRomSize() const { return rom_data.getSize(); }

    // Authentic 88-Key Multisample Table Parameter (from MAME hardware trace)
    struct PianoKeyParam
    {
        uint32_t addr_start;
        uint32_t addr_end;
        uint32_t addr_loop;
        uint32_t pitch;
        uint8_t  exp;
        uint32_t volume_target;
        uint32_t volume_rate;
        uint16_t gain;
    };

    // AP-10 Tone definitions (Envelope characteristics & multisample mapping)
    struct TonePreset
    {
        const char*          name;
        const PianoKeyParam* key_map;
        uint32_t             decay_rate;
        uint32_t             release_rate;
        uint32_t             sustain_level;
    };

    static const PianoKeyParam piano_key_map[88];
    static const PianoKeyParam epiano_key_map[88];
    static const PianoKeyParam harpsichord_key_map[88];
    static const PianoKeyParam organ_key_map[88];
    static const PianoKeyParam strings_key_map[88];

    static const TonePreset presets[5];
    TonePreset mod_presets[5];

    int current_preset = 0;
    bool damper_pedal = false;
    bool soft_pedal = false;

    GT913Engine();
    ~GT913Engine();

    void reset();
    void clearRom();
    bool loadAndDecryptRom(const juce::File& file, bool use_mame_word_swap = false);
    void setTone(int index);
    const TonePreset& getCurrentPreset() const;
    const PianoKeyParam* getKeyMapForCurrentPreset() const;
    void setEnvelopeParams(int presetIndex, uint32_t decay, uint32_t release, uint32_t sustain);

    void noteOn(int midiNote, int velocity);
    void noteOff(int midiNote);
    void setDamperPedal(bool down);
    void setSoftPedal(bool down);

    static uint32_t calculatePitch(int midiNote, int baseNote, uint32_t basePitch);

    void update_envelope(Voice& v);
    void update_sample(Voice& v);
    void mix_sample(Voice& v, int64_t& left, int64_t& right);

private:
    uint16_t read_word(uint32_t addr) const;
};
