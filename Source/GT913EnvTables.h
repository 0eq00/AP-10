// GT913EnvTables.h
// Envelope tables generated from MAME debug logs of the Casio AP-10 firmware
// (gt913_sound_device::command_w, voicecmd 0x6007), all 88 keys, velocity 127.
//
// Each stage is a 16-bit data word written to the sound chip:
//   target = (word & 0x7f00) << 16
//   rate   = see gt913_env_rate() below (same as gt913.cpp command_w)
// The firmware polls the voice about every 40.96 ms (about 24.4 Hz). At each poll,
// when the current volume has reached (or is very close to) the target, it writes the next word.
// On key-off, it writes the release word (target = 0). Words after that are clean-up writes.
#pragma once
#include <cstdint>

struct GT913EnvZone
{
    uint8_t  first_key;   // MIDI note number
    uint8_t  last_key;
    uint8_t  num_words;
    uint16_t words[8];    // note-on sequence, in order
    uint16_t release;     // key-off word
};

inline uint32_t gt913_env_rate(uint16_t data)
{
    uint8_t base = data & 0xff;
    uint8_t shift = base >> 5;
    switch (shift)
    {
    case 0:  shift = base >> 2; base &= 3; break;
    case 1:  shift = 8;         base &= 0x1f; break;
    default: shift += 6;        base = (base & 0x1f) | 0x20; break;
    }
    return (uint32_t)(base * 3) << (shift + 5);
}

inline uint32_t gt913_env_target(uint16_t data) { return (uint32_t)(data & 0x7f00) << 16; }

// Velocity scaling (verified with the velocity sweep logs, keys A0/C4/C8, 10 velocities per tone):
//  - The rate part (low byte) of every word and the release word do not depend on velocity.
//  - The target of every stage scales in proportion to the first-stage target:
//        target_k(v) = round( target_k(127) * T1(v) / T1(127) )     (7-bit units, error is at most 1)
//    T1(v) is the first-stage target for velocity v (the existing velocity_volume_target table).
//  - The words[] below hold the velocity-127 values.
inline uint32_t gt913_env_target_scaled(uint16_t word127, uint32_t t1_v, uint32_t t1_127)
{
    const uint32_t tgt127 = (word127 >> 8) & 0x7f;   // 7-bit target at velocity 127
    if (tgt127 == 0 || t1_127 == 0) return 0;
    const uint32_t a = t1_v >> 24, b = t1_127 >> 24; // 7-bit first-stage targets
    uint32_t scaled = (tgt127 * a + b / 2) / b;
    if (scaled > 0x7f) scaled = 0x7f;
    return scaled << 24;
}

// Grand Piano: 11 zones
static const GT913EnvZone piano_env_zones[] = {
    {  21,  28, 6, { 0x7EFF,0x7C0D,0x7015,0x410F,0x2F11,0x000B }, 0x0023 },
    {  29,  40, 6, { 0x7EFF,0x7C0D,0x6D13,0x4115,0x2F11,0x000B }, 0x0023 },
    {  41,  46, 6, { 0x7EFF,0x7C0D,0x6713,0x3B15,0x2F0F,0x000B }, 0x0023 },
    {  47,  56, 5, { 0x7EFF,0x7C0D,0x6313,0x3115,0x000B }, 0x0023 },
    {  57,  62, 5, { 0x7EFF,0x7D0D,0x5B13,0x3115,0x0011 }, 0x0023 },
    {  63,  75, 5, { 0x7EFF,0x7D0D,0x5B13,0x3315,0x000F }, 0x0023 },
    {  76,  83, 6, { 0x7EFF,0x7D0D,0x6719,0x4E13,0x1F15,0x000F }, 0x0023 },
    {  84,  87, 4, { 0x7EFF,0x7D0D,0x6F19,0x0013 }, 0x0023 },
    {  88,  91, 3, { 0x7EFF,0x5F1D,0x0013 }, 0x0023 },
    {  92,  98, 3, { 0x7EFF,0x5F17,0x0019 }, 0x001D },
    {  99, 108, 3, { 0x7EFF,0x5F1D,0x0019 }, 0x001D },
};

// E. Piano: 9 zones
static const GT913EnvZone ep_env_zones[] = {
    {  21,  39, 6, { 0x76FF,0x7411,0x5D19,0x3B15,0x1D11,0x000B }, 0x0025 },
    {  40,  44, 6, { 0x76FF,0x7415,0x5C17,0x4113,0x1D11,0x000B }, 0x0025 },
    {  45,  55, 6, { 0x76FF,0x7513,0x661D,0x4A19,0x3015,0x0011 }, 0x0025 },
    {  56,  64, 6, { 0x76FF,0x7519,0x661B,0x4E17,0x2513,0x0011 }, 0x0025 },
    {  65,  71, 5, { 0x76FF,0x6721,0x4F17,0x1D13,0x0011 }, 0x0025 },
    {  72,  76, 7, { 0x76FF,0x6B22,0x5521,0x3F1D,0x2819,0x1615,0x0011 }, 0x0025 },
    {  77,  83, 7, { 0x76FF,0x6822,0x5121,0x3F1B,0x2117,0x1215,0x0011 }, 0x0025 },
    {  84,  91, 6, { 0x76FF,0x5C22,0x4621,0x1F1D,0x1215,0x0011 }, 0x0025 },
    {  92, 108, 6, { 0x76FF,0x5922,0x3421,0x1A1D,0x1213,0x000F }, 0x0025 },
};

// Harpsichord: 7 zones
static const GT913EnvZone harps_env_zones[] = {
    {  21,  41, 5, { 0x74FF,0x671B,0x5817,0x3A13,0x0015 }, 0x0026 },
    {  42,  50, 5, { 0x74FF,0x6021,0x4A17,0x3319,0x0013 }, 0x0026 },
    {  51,  59, 5, { 0x74FF,0x601F,0x4A1D,0x3317,0x0013 }, 0x0026 },
    {  60,  68, 5, { 0x74FF,0x621F,0x501D,0x3317,0x0013 }, 0x0026 },
    {  69,  78, 5, { 0x74FF,0x5F22,0x5021,0x331D,0x0013 }, 0x0026 },
    {  79,  86, 4, { 0x74FF,0x5022,0x3321,0x0019 }, 0x0026 },
    {  87, 108, 4, { 0x74FF,0x5024,0x3321,0x0017 }, 0x0026 },
};

// Pipe Organ: 1 zones
static const GT913EnvZone organ_env_zones[] = {
    {  21, 108, 3, { 0x705E,0x6D0B,0x6907 }, 0x0023 },
};

// Strings: 1 zones
static const GT913EnvZone strings_env_zones[] = {
    {  21, 108, 3, { 0x5656,0x7030,0x660D }, 0x001F },
};
