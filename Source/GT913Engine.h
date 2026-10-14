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

// Firmware envelope zone (defined in GT913EnvTables.h, included from GT913Engine.cpp)
struct GT913EnvZone;

class GT913Engine
{
public:
    static constexpr int MAX_ALLOCATED_VOICES = 88;
    static constexpr int DEFAULT_VOICES = 24;
    static constexpr int MAX_VOICES = MAX_ALLOCATED_VOICES;

    int      max_voices = DEFAULT_VOICES;
    int      getMaxVoices() const { return max_voices; }
    void     setMaxVoices(int count);
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
        int      m_preset_id = 0; // 0: Piano, 1: E.Piano, 2: Harpsichord, 3: Organ, 4: Strings
        uint32_t m_release_rate = 0x00080000;

        // Firmware-style multi-stage envelope (see GT913EnvTables.h)
        const GT913EnvZone* m_env_zone = nullptr; // null = legacy decay/sustain/release envelope
        int      m_env_index = 0;                 // index of the current stage word
        uint32_t m_t1_v = 0;                      // first-stage target at this velocity
        uint32_t m_t1_127 = 0;                    // first-stage target at velocity 127
        uint32_t m_poll_timer = 0;                // ticks until the next firmware poll
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
        uint32_t unused_volume_target;
        uint32_t unused_volume_rate;
        uint16_t gain;
    };

    // AP-10 Tone definitions (Envelope characteristics & multisample mapping)
    struct TonePreset
    {
        const char*          name;
        const PianoKeyParam* key_map;
    };

    static const PianoKeyParam piano_key_map[88];
    static const PianoKeyParam epiano_key_map[88];
    static const PianoKeyParam harpsichord_key_map[88];
    static const PianoKeyParam organ_key_map[88];
    static const PianoKeyParam strings_key_map[88];

    // Authentic Velocity-to-Target Volume mapping (from 11.Velocity.csv)
    // 5 Presets x 128 Velocity steps (0..127)
    static const uint32_t velocity_volume_target[5][128];
    static uint32_t getVelocityVolumeTarget(int presetIndex, int velocity);

    // Preset calibration volume trim multipliers (Index 0: Piano, 1: E.Piano, 2: Harpsichord, 3: Organ, 4: Strings)
    // Matched to MAME AP-10 recording loudness ratios:
    // Piano:       +0.0 dB (1.000x)
    // E.Piano:     -5.1 dB (0.556x)
    // Harpsichord: -3.5 dB (0.668x)
    // Pipe Organ:  -1.1 dB (0.881x)
    // Strings:     +4.5 dB (1.679x)
    const float preset_volume_gain[5] = { 1.04f, 1.09f, 1.04f, 1.05f, 1.04f };

    // Every voice follows the multi-stage
    // envelope logged from the MAME AP-10 firmware (GT913EnvTables.h) and the decay, sustain
    // and release values in presets[] are ignored. Switching also swaps the preset volume gains.
    static constexpr uint32_t ENV_POLL_PERIOD_TICKS   = 4689; // 40.96 ms at 114483 Hz
    static constexpr uint32_t ENV_ADVANCE_MARGIN_TICKS = 0;    // 0: advance only when the target is reached (best match to the logs)
    bool     poll_phase_random = false;   // false: fixed phase (half a period), true: random phase per note
    uint32_t poll_phase_ticks = ENV_POLL_PERIOD_TICKS / 2; // used when poll_phase_random is false

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
    float    release_multiplier = 1.0f; // Multiplies firmware envelope release rate
    void     setReleaseMultiplier(float mult);
    float    getReleaseMultiplier() const { return release_multiplier; }
    float    piano_high_release_multiplier = 1.0f; // Multiplier for Grand Piano key >= 92 (G#6..C8)
    void     setPianoHighReleaseMultiplier(float mult);
    float    getPianoHighReleaseMultiplier() const { return piano_high_release_multiplier; }
    float    getEffectiveReleaseMultiplier(int presetIndex, int midiNote) const;
    void setEnvelopeParams(int presetIndex, uint32_t decay, uint32_t release, uint32_t sustain);

    void noteOn(int midiNote, int velocity);
    void noteOff(int midiNote);
    void setDamperPedal(bool down);
    void setSoftPedal(bool down);

    static uint32_t calculatePitch(int midiNote, int baseNote, uint32_t basePitch);

    void update_envelope(Voice& v);
    const GT913EnvZone* findEnvZone(int presetIndex, int midiNote) const;
    void update_sample(Voice& v);
    void mix_sample(Voice& v, int64_t& left, int64_t& right);

private:
    uint16_t read_word(uint32_t addr) const;
    uint32_t nextPollPhaseTicks();
    float legacy_preset_volume_gain[5] = { 1.000f, 0.556f, 0.668f, 0.881f, 1.679f };
    uint32_t m_phase_seed = 0x2545F491u;
};
