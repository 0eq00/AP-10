/*
  ==============================================================================
    GT913Engine.h
    Casio GT913 Sound Engine & 24-Voice ADPCM Processor for JUCE
    Ported & adapted from MAME gt913.cpp & ctk551.cpp (Celviano AP-10)
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
    static inline const uint8_t exp_2_to_3[4] = { 0, 1, 2, 7 };

    static inline const int8_t sample_7_to_8[128] = {
          0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
         16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
         32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
         48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
        -64, -63, -62, -61, -60, -59, -58, -57, -56, -55, -54, -53, -52, -51, -50, -49,
        -48, -47, -46, -45, -44, -43, -42, -41, -40, -39, -38, -37, -36, -35, -34, -33,
        -32, -31, -30, -29, -28, -27, -26, -25, -24, -23, -22, -21, -20, -19, -18, -17,
        -16, -15, -14, -13, -12, -11, -10,  -9,  -8,  -7,  -6,  -5,  -4,  -3,  -2,  -1
    };

    static inline const uint16_t volume_ramp[17] = {
        0x0000, 0x00fa, 0x0231, 0x03b5, 0x0596, 0x07ee, 0x0ad8, 0x0e78,
        0x12fa, 0x1897, 0x1f93, 0x2843, 0x3313, 0x4087, 0x5143, 0x6617,
        0x8000
    };

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
        int      m_env_stage = 0; // 0: idle, 1: attack, 2: decay, 3: release
        uint32_t m_release_rate = 3500;
        uint32_t m_sustain_level = 0;
    };

    Voice voices[MAX_VOICES];

    // ROM buffer stored directly inside the engine instance
    juce::MemoryBlock rom_data;
    bool rom_loaded = false;

    bool isRomLoaded() const { return rom_loaded; }
    size_t getRomSize() const { return rom_data.getSize(); }

    // AP-10 Tone definitions
    struct TonePreset
    {
        const char* name;
        uint32_t addr_start;
        uint32_t addr_end;
        uint32_t addr_loop;
        uint32_t base_pitch;
        int      base_note;
        uint16_t gain;
        uint32_t attack_rate;
        uint32_t decay_rate;
        uint32_t release_rate;
        uint32_t sustain_level;
    };

    // Authentic 88-Key Piano Multisample Table (from MAME hardware trace)
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

    static inline const PianoKeyParam piano_key_map[88] = {
        { 0x020000, 0x030732, 0x02A3D3, 0x053D772, 0x00, 0x7E000000, 0x02F40000, 0x0C }, // Note 21 (A0)
        { 0x020000, 0x030732, 0x02A3D3, 0x058EDAC, 0x00, 0x7E000000, 0x02F40000, 0x0C }, // Note 22
        { 0x020000, 0x030732, 0x02A3D3, 0x05E4CD6, 0x00, 0x7E000000, 0x02F40000, 0x0C }, // Note 23
        { 0x020000, 0x030732, 0x02A3D3, 0x063FFCC, 0x00, 0x7E000000, 0x02F40000, 0x0C }, // Note 24
        { 0x020000, 0x030732, 0x02A3D3, 0x06A09EA, 0x00, 0x7E000000, 0x02F40000, 0x0C }, // Note 25
        { 0x020000, 0x030732, 0x02A3D3, 0x07072BA, 0x00, 0x7E000000, 0x02F40000, 0x0C }, // Note 26
        { 0x020000, 0x030732, 0x02A3D3, 0x0773DA0, 0x00, 0x7E000000, 0x02F40000, 0x0C }, // Note 27
        { 0x020000, 0x030732, 0x02A3D3, 0x07E6AA6, 0x00, 0x7E000000, 0x02F40000, 0x0C }, // Note 28
        { 0x03073E, 0x0400C6, 0x03AA58, 0x05E9934, 0x00, 0x7E000000, 0x02F40000, 0x0B }, // Note 29 (F1)
        { 0x03073E, 0x0400C6, 0x03AA58, 0x0644AB6, 0x00, 0x7E000000, 0x02F40000, 0x0B }, // Note 30
        { 0x03073E, 0x0400C6, 0x03AA58, 0x06A53BA, 0x00, 0x7E000000, 0x02F40000, 0x0B }, // Note 31
        { 0x03073E, 0x0400C6, 0x03AA58, 0x070B37A, 0x00, 0x7E000000, 0x02F40000, 0x0B }, // Note 32
        { 0x03073E, 0x0400C6, 0x03AA58, 0x07774DC, 0x00, 0x7E000000, 0x02F40000, 0x0B }, // Note 33
        { 0x0400D2, 0x04B08C, 0x0464B1, 0x05969D0, 0x00, 0x7E000000, 0x02F40000, 0x0D }, // Note 34 (Bb1)
        { 0x0400D2, 0x04B08C, 0x0464B1, 0x05EC50A, 0x00, 0x7E000000, 0x02F40000, 0x0D }, // Note 35
        { 0x0400D2, 0x04B08C, 0x0464B1, 0x0646DEC, 0x00, 0x7E000000, 0x02F40000, 0x0D }, // Note 36
        { 0x0400D2, 0x04B08C, 0x0464B1, 0x06A71E2, 0x00, 0x7E000000, 0x02F40000, 0x0D }, // Note 37
        { 0x0400D2, 0x04B08C, 0x0464B1, 0x070CD8C, 0x00, 0x7E000000, 0x02F40000, 0x0D }, // Note 38
        { 0x0400D2, 0x04B08C, 0x0464B1, 0x077895A, 0x00, 0x7E000000, 0x02F40000, 0x0D }, // Note 39
        { 0x0400D2, 0x04B08C, 0x0464B1, 0x07EACB6, 0x00, 0x7E000000, 0x02F40000, 0x0D }, // Note 40
        { 0x04B098, 0x055C46, 0x051A45, 0x05997DE, 0x01, 0x7E000000, 0x02F40000, 0x0E }, // Note 41 (F2)
        { 0x04B098, 0x055C46, 0x051A45, 0x05EEB38, 0x01, 0x7E000000, 0x02F40000, 0x0E }, // Note 42
        { 0x04B098, 0x055C46, 0x051A45, 0x0649660, 0x01, 0x7E000000, 0x02F40000, 0x0E }, // Note 43
        { 0x04B098, 0x055C46, 0x051A45, 0x06A90D6, 0x01, 0x7E000000, 0x02F40000, 0x0E }, // Note 44
        { 0x04B098, 0x055C46, 0x051A45, 0x070E79C, 0x01, 0x7E000000, 0x02F40000, 0x0E }, // Note 45
        { 0x04B098, 0x055C46, 0x051A45, 0x0779DD8, 0x01, 0x7E000000, 0x02F40000, 0x0E }, // Note 46
        { 0x055C52, 0x06156A, 0x05D757, 0x05EE09E, 0x00, 0x7E000000, 0x02F40000, 0x0E }, // Note 47 (B2)
        { 0x055C52, 0x06156A, 0x05D757, 0x0648524, 0x00, 0x7E000000, 0x02F40000, 0x0E }, // Note 48
        { 0x055C52, 0x06156A, 0x05D757, 0x06A7E96, 0x00, 0x7E000000, 0x02F40000, 0x0E }, // Note 49
        { 0x055C52, 0x06156A, 0x05D757, 0x070D446, 0x00, 0x7E000000, 0x02F40000, 0x0E }, // Note 50
        { 0x055C52, 0x06156A, 0x05D757, 0x077895A, 0x00, 0x7E000000, 0x02F40000, 0x0E }, // Note 51
        { 0x061576, 0x06BB2E, 0x0681E7, 0x05EE09E, 0x01, 0x7E000000, 0x02F40000, 0x0E }, // Note 52 (E3)
        { 0x061576, 0x06BB2E, 0x0681E7, 0x0648524, 0x01, 0x7E000000, 0x02F40000, 0x0E }, // Note 53
        { 0x061576, 0x06BB2E, 0x0681E7, 0x06A7E96, 0x01, 0x7E000000, 0x02F40000, 0x0E }, // Note 54
        { 0x061576, 0x06BB2E, 0x0681E7, 0x070D446, 0x01, 0x7E000000, 0x02F40000, 0x0E }, // Note 55
        { 0x061576, 0x06BB2E, 0x0681E7, 0x077895A, 0x01, 0x7E000000, 0x02F40000, 0x0E }, // Note 56
        { 0x061576, 0x06BB2E, 0x0681E7, 0x07EA52A, 0x01, 0x7E000000, 0x02F40000, 0x0E }, // Note 57
        { 0x06BB3A, 0x07585C, 0x072345, 0x05997DE, 0x01, 0x7E000000, 0x02F40000, 0x0F }, // Note 58 (Bb3)
        { 0x06BB3A, 0x07585C, 0x072345, 0x05EEB38, 0x01, 0x7E000000, 0x02F40000, 0x0F }, // Note 59
        { 0x06BB3A, 0x07585C, 0x072345, 0x0649062, 0x01, 0x7E000000, 0x02F40000, 0x0F }, // Note 60 (C4)
        { 0x06BB3A, 0x07585C, 0x072345, 0x06A8A7C, 0x01, 0x7E000000, 0x02F40000, 0x0F }, // Note 61
        { 0x06BB3A, 0x07585C, 0x072345, 0x070E0E2, 0x01, 0x7E000000, 0x02F40000, 0x0F }, // Note 62
        { 0x06BB3A, 0x07585C, 0x072345, 0x07796B6, 0x01, 0x7E000000, 0x02F40000, 0x0F }, // Note 63
        { 0x075868, 0x07E606, 0x07AF10, 0x05973D4, 0x02, 0x7E000000, 0x02F40000, 0x0F }, // Note 64 (E4)
        { 0x075868, 0x07E606, 0x07AF10, 0x05EC50A, 0x02, 0x7E000000, 0x02F40000, 0x0F }, // Note 65
        { 0x075868, 0x07E606, 0x07AF10, 0x06467EE, 0x02, 0x7E000000, 0x02F40000, 0x0F }, // Note 66
        { 0x075868, 0x07E606, 0x07AF10, 0x06A5FA2, 0x02, 0x7E000000, 0x02F40000, 0x0F }, // Note 67
        { 0x075868, 0x07E606, 0x07AF10, 0x070B37A, 0x02, 0x7E000000, 0x02F40000, 0x0F }, // Note 68
        { 0x075868, 0x07E606, 0x07AF10, 0x077669C, 0x02, 0x7E000000, 0x02F40000, 0x0F }, // Note 69
        { 0x07E612, 0x0883B0, 0x085746, 0x05EBF62, 0x00, 0x7E000000, 0x02F40000, 0x0F }, // Note 70 (Bb4)
        { 0x07E612, 0x0883B0, 0x085746, 0x06461F0, 0x00, 0x7E000000, 0x02F40000, 0x0F }, // Note 71
        { 0x07E612, 0x0883B0, 0x085746, 0x06A5948, 0x00, 0x7E000000, 0x02F40000, 0x0F }, // Note 72
        { 0x07E612, 0x0883B0, 0x085746, 0x070ACC0, 0x00, 0x7E000000, 0x02F40000, 0x0F }, // Note 73
        { 0x07E612, 0x0883B0, 0x085746, 0x0775F7A, 0x00, 0x7E000000, 0x02F40000, 0x0F }, // Note 74
        { 0x07E612, 0x0883B0, 0x085746, 0x07E78CE, 0x00, 0x7E000000, 0x02F40000, 0x0F }, // Note 75
        { 0x0883BC, 0x090D70, 0x08E8FC, 0x0594A72, 0x02, 0x7E000000, 0x02F40000, 0x0F }, // Note 76 (E5)
        { 0x0883BC, 0x090D70, 0x08E8FC, 0x05E9934, 0x02, 0x7E000000, 0x02F40000, 0x0F }, // Note 77
        { 0x0883BC, 0x090D70, 0x08E8FC, 0x064397C, 0x02, 0x7E000000, 0x02F40000, 0x0F }, // Note 78
        { 0x0883BC, 0x090D70, 0x08E8FC, 0x06A2E6C, 0x02, 0x7E000000, 0x02F40000, 0x0F }, // Note 79
        { 0x0883BC, 0x090D70, 0x08E8FC, 0x0708612, 0x02, 0x7E000000, 0x02F40000, 0x0F }, // Note 80
        { 0x0883BC, 0x090D70, 0x08E8FC, 0x0773680, 0x02, 0x7E000000, 0x02F40000, 0x0F }, // Note 81
        { 0x0883BC, 0x090D70, 0x08E8FC, 0x07E4D62, 0x02, 0x7E000000, 0x02F40000, 0x0F }, // Note 82
        { 0x0883BC, 0x090D70, 0x08E8FC, 0x085D900, 0x02, 0x7E000000, 0x02F40000, 0x0F }, // Note 83
        { 0x090D7C, 0x095F5A, 0x094299, 0x06A53BA, 0x02, 0x7E000000, 0x02F40000, 0x10 }, // Note 84 (C6)
        { 0x090D7C, 0x095F5A, 0x094299, 0x070ACC0, 0x02, 0x7E000000, 0x02F40000, 0x10 }, // Note 85
        { 0x090D7C, 0x095F5A, 0x094299, 0x077669C, 0x02, 0x7E000000, 0x02F40000, 0x10 }, // Note 86
        { 0x090D7C, 0x095F5A, 0x094299, 0x07E805A, 0x02, 0x7E000000, 0x02F40000, 0x10 }, // Note 87
        { 0x095F66, 0x09C346, 0x09A295, 0x0592110, 0x00, 0x7E000000, 0x02F40000, 0x0F }, // Note 88 (E6)
        { 0x095F66, 0x09C346, 0x09A295, 0x05E7304, 0x00, 0x7E000000, 0x02F40000, 0x0F }, // Note 89
        { 0x095F66, 0x09C346, 0x09A295, 0x0641D04, 0x00, 0x7E000000, 0x02F40000, 0x0F }, // Note 90
        { 0x095F66, 0x09C346, 0x09A295, 0x06A15D2, 0x00, 0x7E000000, 0x02F40000, 0x0F }, // Note 91
        { 0x095F66, 0x09C346, 0x09A295, 0x0706C00, 0x00, 0x7E000000, 0x02F40000, 0x0F }, // Note 92
        { 0x095F66, 0x09C346, 0x09A295, 0x0772840, 0x00, 0x7E000000, 0x02F40000, 0x0F }, // Note 93
        { 0x09C352, 0x0A1B92, 0x09FE41, 0x0592668, 0x02, 0x7E000000, 0x02F40000, 0x1F }, // Note 94 (Bb6)
        { 0x09C352, 0x0A1B92, 0x09FE41, 0x05E7E54, 0x02, 0x7E000000, 0x02F40000, 0x1F }, // Note 95
        { 0x09C352, 0x0A1B92, 0x09FE41, 0x0642840, 0x02, 0x7E000000, 0x02F40000, 0x1F }, // Note 96
        { 0x09C352, 0x0A1B92, 0x09FE41, 0x06A2E6C, 0x02, 0x7E000000, 0x02F40000, 0x1F }, // Note 97
        { 0x09C352, 0x0A1B92, 0x09FE41, 0x0708CCC, 0x02, 0x7E000000, 0x02F40000, 0x1F }, // Note 98
        { 0x09C352, 0x0A1B92, 0x09FE41, 0x077521E, 0x02, 0x7E000000, 0x02F40000, 0x1F }, // Note 99
        { 0x0A1B9E, 0x0A6076, 0x0A4760, 0x058CA4C, 0x01, 0x7E000000, 0x02F40000, 0x1F }, // Note 100 (E7)
        { 0x0A1B9E, 0x0A6076, 0x0A4760, 0x05E26A6, 0x01, 0x7E000000, 0x02F40000, 0x1F }, // Note 101
        { 0x0A1B9E, 0x0A6076, 0x0A4760, 0x063D758, 0x01, 0x7E000000, 0x02F40000, 0x1F }, // Note 102
        { 0x0A1B9E, 0x0A6076, 0x0A4760, 0x069DF0E, 0x01, 0x7E000000, 0x02F40000, 0x1F }, // Note 103
        { 0x0A1B9E, 0x0A6076, 0x0A4760, 0x0704552, 0x01, 0x7E000000, 0x02F40000, 0x1F }, // Note 104
        { 0x0A1B9E, 0x0A6076, 0x0A4760, 0x07713C2, 0x01, 0x7E000000, 0x02F40000, 0x1F }, // Note 105
        { 0x0A1B9E, 0x0A6076, 0x0A4760, 0x07E4D62, 0x01, 0x7E000000, 0x02F40000, 0x1F }, // Note 106
        { 0x0A1B9E, 0x0A6076, 0x0A4760, 0x085F800, 0x01, 0x7E000000, 0x02F40000, 0x1F }, // Note 107
        { 0x0A1B9E, 0x0A6076, 0x0A4760, 0x08E1F92, 0x01, 0x7E000000, 0x02F40000, 0x1F }  // Note 108 (C8)
    };

    static inline const TonePreset presets[5] = {
        { "Grand Piano",  0x06BB3A, 0x07585C, 0x072345, 0x00649062, 60, 0x0F, 0x08000000, 0x00001000, 0x00080000, 0 },
        { "E. Piano",     0x07586C, 0x07DF00, 0x07CA00, 0x00649062, 60, 0x10, 0x0A000000, 0x00000C00, 0x00070000, 0x15000000 },
        { "Harpsichord",  0x07DF10, 0x085200, 0x083800, 0x00649062, 60, 0x0E, 0x0C000000, 0x00002000, 0x000A0000, 0 },
        { "Pipe Organ",   0x085210, 0x08D800, 0x08B000, 0x00649062, 60, 0x0D, 0x05000000, 0x00000200, 0x00050000, 0x50000000 },
        { "Strings",      0x08D810, 0x098A00, 0x094800, 0x00649062, 60, 0x0C, 0x02000000, 0x00000100, 0x00040000, 0x58000000 }
    };

    int current_preset = 0;

    // Active patch parameters
    uint32_t patch_addr_start = 0x06BB3A;
    uint32_t patch_addr_end   = 0x07585C;
    uint32_t patch_addr_loop  = 0x072345;
    uint32_t patch_base_pitch = 0x00649062;
    int      patch_base_note  = 60;
    uint16_t patch_gain       = 0x0F;
    uint32_t patch_attack_rate  = 0x08000000;
    uint32_t patch_decay_rate   = 0x00001000;
    uint32_t patch_release_rate = 0x00080000;
    uint32_t patch_sustain_level = 0;

    bool damper_pedal = false;
    bool soft_pedal   = false;

    GT913Engine()
    {
        setTone(0);
        reset();
    }

    void setTone(int index)
    {
        if (index < 0 || index >= 5) return;
        current_preset = index;
        const auto& p = presets[index];
        patch_addr_start  = p.addr_start;
        patch_addr_end    = p.addr_end;
        patch_addr_loop   = p.addr_loop;
        patch_base_pitch  = p.base_pitch;
        patch_base_note   = p.base_note;
        patch_gain        = p.gain;
        patch_attack_rate = p.attack_rate;
        patch_decay_rate  = p.decay_rate;
        patch_release_rate= p.release_rate;
        patch_sustain_level = p.sustain_level;
    }

    void reset()
    {
        for (int i = 0; i < MAX_VOICES; ++i)
        {
            voices[i] = Voice();
        }
    }

    uint16_t read_word(uint32_t addr) const
    {
        if (!rom_loaded || addr + 1 >= rom_data.getSize())
            return 0;

        const uint8_t* ptr = static_cast<const uint8_t*>(rom_data.getData());
        return (static_cast<uint16_t>(ptr[addr + 1]) << 8) | ptr[addr];
    }

    void clearRom()
    {
        // Critical: setSize(0) resets the block size to 0 bytes.
        // Calling reset() only zeroes bytes without resizing, which caused
        // loadFileAsData to append another 1024 KB resulting in the 2048 KB bug!
        rom_data.setSize(0);
        rom_loaded = false;
        reset();
    }

    // Original clean ROM loader and descrambler
    // Default use_mame_word_swap = false preserves the clean, authentic prototype audio
    bool loadAndDecryptRom(const juce::File& file, bool use_mame_word_swap = false)
    {
        if (!file.existsAsFile())
            return false;

        // Reset previous ROM contents completely (setSize(0)) to prevent appending (2048 KB bug)
        clearRom();

        file.loadFileAsData(rom_data);
        if (rom_data.getSize() < 1024)
        {
            clearRom();
            return false;
        }

        uint8_t* ptr = static_cast<uint8_t*>(rom_data.getData());
        const size_t half_size = rom_data.getSize() >> 1;

        for (size_t addr = 0; addr < half_size; ++addr)
        {
            size_t byte_idx = addr * 2;
            uint16_t swapped_val = 0;

            if (use_mame_word_swap)
                swapped_val = (static_cast<uint16_t>(ptr[byte_idx]) << 8) | ptr[byte_idx + 1];
            else
                swapped_val = (static_cast<uint16_t>(ptr[byte_idx + 1]) << 8) | ptr[byte_idx];

            uint16_t decrypted = 0;
            decrypted |= ((swapped_val >> 15) & 1) << 15;
            decrypted |= ((swapped_val >> 14) & 1) << 14;
            decrypted |= ((swapped_val >> 13) & 1) << 13;
            decrypted |= ((swapped_val >> 10) & 1) << 12;
            decrypted |= ((swapped_val >> 11) & 1) << 11;
            decrypted |= ((swapped_val >> 12) & 1) << 10;
            decrypted |= ((swapped_val >> 9)  & 1) << 9;
            decrypted |= ((swapped_val >> 8)  & 1) << 8;
            decrypted |= ((swapped_val >> 7)  & 1) << 7;
            decrypted |= ((swapped_val >> 6)  & 1) << 6;
            decrypted |= ((swapped_val >> 2)  & 1) << 5;
            decrypted |= ((swapped_val >> 3)  & 1) << 4;
            decrypted |= ((swapped_val >> 4)  & 1) << 3;
            decrypted |= ((swapped_val >> 5)  & 1) << 2;
            decrypted |= ((swapped_val >> 1)  & 1) << 1;
            decrypted |= ((swapped_val >> 0)  & 1) << 0;

            ptr[byte_idx]     = decrypted & 0xFF;
            ptr[byte_idx + 1] = (decrypted >> 8) & 0xFF;
        }

        rom_loaded = true;
        return true;
    }

    // Calculate GT913 25-bit phase accumulator pitch for any MIDI note
    uint32_t calculatePitch(int midiNote) const
    {
        // P = P0 * 2^((note - note0) / 12)
        double semitoneDiff = static_cast<double>(midiNote - patch_base_note);
        double multiplier = std::pow(2.0, semitoneDiff / 12.0);
        double calculated = static_cast<double>(patch_base_pitch) * multiplier;
        return static_cast<uint32_t>(std::clamp(calculated, 1000.0, static_cast<double>(0x01FFFFFF)));
    }

    void noteOn(int midiNote, int velocity)
    {
        int targetIdx = -1;

        // 1. Check if same note is already sounding (retrigger)
        for (int i = 0; i < MAX_VOICES; ++i)
        {
            if (voices[i].m_enable && voices[i].m_midi_note == midiNote)
            {
                targetIdx = i;
                break;
            }
        }

        // 2. Find free voice
        if (targetIdx == -1)
        {
            for (int i = 0; i < MAX_VOICES; ++i)
            {
                if (!voices[i].m_enable)
                {
                    targetIdx = i;
                    break;
                }
            }
        }

        // 3. Voice stealing (oldest in release or lowest volume)
        if (targetIdx == -1)
        {
            uint32_t lowestVol = 0xFFFFFFFF;
            for (int i = 0; i < MAX_VOICES; ++i)
            {
                uint32_t v = voices[i].m_volume_current;
                if (voices[i].m_is_releasing) v /= 4;
                if (v < lowestVol)
                {
                    lowestVol = v;
                    targetIdx = i;
                }
            }
        }

        if (targetIdx == -1) targetIdx = 0;

        auto& v = voices[targetIdx];
        v.m_enable = true;
        v.m_midi_note = midiNote;
        v.m_velocity = velocity;
        v.m_is_releasing = false;

        float velNorm = std::clamp(static_cast<float>(velocity) / 127.0f, 0.1f, 1.0f);

        // Authentic Casio AP-10 88-Key Piano Multisample Table (MAME hardware trace)
        if (current_preset == 0 && midiNote >= 21 && midiNote <= 108)
        {
            const auto& keyParam = piano_key_map[midiNote - 21];
            v.m_addr_start   = keyParam.addr_start;
            v.m_addr_end     = keyParam.addr_end;
            v.m_addr_loop    = keyParam.addr_loop;
            v.m_addr_current = v.m_addr_start;
            v.m_addr_frac    = 0;
            v.m_sample       = 0;
            v.m_sample_next  = 0;
            v.m_exp          = keyParam.exp;
            v.m_pitch        = keyParam.pitch;
            v.m_gain         = keyParam.gain;

            uint32_t peakVol = static_cast<uint32_t>(keyParam.volume_target * (0.3f + 0.7f * velNorm));
            v.m_volume_current = 0;
            v.m_volume_target  = peakVol;
            v.m_volume_rate    = keyParam.volume_rate;
            v.m_env_stage      = 1; // Attack
            v.m_balance[0]     = 7;
            v.m_balance[1]     = 7;
            v.m_release_rate   = patch_release_rate;
            v.m_sustain_level  = patch_sustain_level;
        }
        else
        {
            v.m_addr_start   = patch_addr_start;
            v.m_addr_end     = patch_addr_end;
            v.m_addr_loop    = patch_addr_loop;
            v.m_addr_current = v.m_addr_start;
            v.m_addr_frac    = 0;
            v.m_sample       = 0;
            v.m_sample_next  = 0;
            v.m_exp          = 0;

            v.m_pitch = calculatePitch(midiNote);

            uint32_t peakVol = static_cast<uint32_t>(0x78000000 * (0.3f + 0.7f * velNorm));
            v.m_volume_current = 0;
            v.m_volume_target  = peakVol;
            v.m_volume_rate    = patch_attack_rate;
            v.m_env_stage      = 1; // Attack
            v.m_gain           = patch_gain;
            v.m_balance[0]     = 7;
            v.m_balance[1]     = 7;
            v.m_release_rate   = patch_release_rate;
            v.m_sustain_level  = patch_sustain_level;
        }
    }

    void noteOff(int midiNote)
    {
        for (int i = 0; i < MAX_VOICES; ++i)
        {
            if (voices[i].m_enable && voices[i].m_midi_note == midiNote && !voices[i].m_is_releasing)
            {
                voices[i].m_is_releasing = true;
                if (!damper_pedal)
                {
                    voices[i].m_env_stage     = 4; // Release
                    voices[i].m_volume_target = 0;
                    voices[i].m_volume_rate   = voices[i].m_release_rate;
                }
            }
        }
    }

    void setDamperPedal(bool down)
    {
        damper_pedal = down;
        if (!down)
        {
            for (int i = 0; i < MAX_VOICES; ++i)
            {
                if (voices[i].m_enable && voices[i].m_is_releasing && voices[i].m_env_stage != 4)
                {
                    voices[i].m_env_stage     = 4;
                    voices[i].m_volume_target = 0;
                    voices[i].m_volume_rate   = voices[i].m_release_rate;
                }
            }
        }
    }

    void setSoftPedal(bool down)
    {
        soft_pedal = down;
    }

    void update_envelope(Voice& v)
    {
        if (!v.m_enable) return;

        if (v.m_env_stage == 1) // Attack
        {
            v.m_volume_current += v.m_volume_rate;
            if (v.m_volume_current >= v.m_volume_target)
            {
                v.m_volume_current = v.m_volume_target;
                v.m_env_stage = 2; // Decay
                v.m_volume_rate = patch_decay_rate;
            }
        }
        else if (v.m_env_stage == 2) // Decay towards sustain level
        {
            if (v.m_volume_current > v.m_sustain_level + v.m_volume_rate)
            {
                v.m_volume_current -= v.m_volume_rate;
            }
            else
            {
                v.m_volume_current = v.m_sustain_level;
                if (v.m_sustain_level == 0)
                {
                    v.m_volume_current = 0;
                    v.m_enable = false;
                    v.m_env_stage = 0;
                }
                else
                {
                    v.m_env_stage = 3; // Sustain stage
                }
            }
        }
        else if (v.m_env_stage == 3) // Sustain (held until note off)
        {
            v.m_volume_current = v.m_sustain_level;
        }
        else if (v.m_env_stage == 4) // Release
        {
            if (v.m_volume_current > v.m_release_rate)
                v.m_volume_current -= v.m_release_rate;
            else
            {
                v.m_volume_current = 0;
                v.m_enable = false;
                v.m_env_stage = 0;
            }
        }

        if (v.m_volume_current <= 50)
        {
            v.m_volume_current = 0;
            v.m_enable = false;
            v.m_env_stage = 0;
        }
    }

    void update_sample(Voice& v)
    {
        v.m_sample += v.m_sample_next;
        // Clamp accumulator to 16-bit range to prevent DC runaway
        v.m_sample = std::clamp(v.m_sample, -32768, 32767);

        if (v.m_addr_current >= v.m_addr_end)
        {
            if (v.m_addr_loop == v.m_addr_end)
            {
                v.m_enable = false;
                return;
            }

            v.m_addr_current = v.m_addr_loop;
            const uint32_t addr_loop_data = (v.m_addr_end + 1) & ~1;
            // Loop target in ROM is signed 16-bit PCM (-32768..32767)
            const int16_t loop_sample = static_cast<int16_t>(read_word(addr_loop_data));
            v.m_sample_next = static_cast<int32_t>(loop_sample) - v.m_sample;
            v.m_exp = read_word(addr_loop_data + 10) & 7;

            if (!(v.m_addr_current & 1))
            {
                const uint16_t word = read_word(v.m_addr_current & ~1);
                const int16_t delta = sample_7_to_8[(word >> 2) & 0x7F];
                v.m_sample_next -= delta * (1 << v.m_exp);
            }
        }
        else
        {
            const uint16_t word = read_word(v.m_addr_current & ~1);
            int16_t delta = 0;

            if (!(v.m_addr_current & 1))
            {
                // Clamped exponent adaptation prevents 128x underflow explosion (the primary cause of harsh noise)
                static const int8_t exp_diff[4] = { 0, 1, 2, -1 };
                int new_exp = static_cast<int>(v.m_exp) + exp_diff[word & 3];
                v.m_exp = static_cast<uint8_t>(std::clamp(new_exp, 0, 7));
                delta = sample_7_to_8[(word >> 2) & 0x7F];
            }
            else
            {
                delta = sample_7_to_8[word >> 9];
            }

            v.m_sample_next = static_cast<int32_t>(delta) * (1 << v.m_exp);
        }

        v.m_addr_current++;
    }

    void mix_sample(Voice& v, int64_t& left, int64_t& right)
    {
        v.m_addr_frac += v.m_pitch;
        while (v.m_enable && v.m_addr_frac >= (1 << 25))
        {
            v.m_addr_frac -= (1 << 25);
            update_sample(v);
        }

        const uint8_t step = (v.m_addr_frac >> 22) & 7;
        const uint8_t env  = (v.m_volume_current >> 27) & 0x0F;
        const uint16_t env_step = (v.m_volume_current >> 16) & 0x7FF;
        const uint32_t env_level = static_cast<uint32_t>(volume_ramp[env]) +
            (((volume_ramp[env + 1] - volume_ramp[env]) * env_step) >> 11);

        const int64_t sample = (static_cast<int64_t>(v.m_sample) + (v.m_sample_next * step / 8)) * v.m_gain * env_level;

        left  += sample * v.m_balance[0];
        right += sample * v.m_balance[1];
    }
};
