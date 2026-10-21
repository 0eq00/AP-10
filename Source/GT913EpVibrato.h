/*
  ==============================================================================
    GT913EpVibrato.h
    Pitch modulation (vibrato) that the Casio AP-10 firmware applies to the E.Piano.
    Self-contained: no other plugin code is needed to use it.

    The firmware rewrites the voice pitch (command 0x4005) at every poll of the voice,
    about every 40.96 ms, 10.2 ms after the envelope poll. The values were logged with MAME
    for all 88 keys at velocity 127. They are offsets from the note-on pitch in ppm,
    so they scale with the pitch. The depth and the shape depend on the key range.
    All comments are 100% English ASCII.
  ==============================================================================
*/

#pragma once

#include <cstdint>

namespace GT913EpVibrato
{
    // Ticks of the engine (115384.6 Hz)
    constexpr uint32_t POLL_PERIOD_TICKS = 4726;   // 40.96 ms
    constexpr uint32_t POLL_OFFSET_TICKS = 1177;   // the pitch poll comes 10.2 ms after the envelope poll
    constexpr uint32_t EARLY_POLL_TICKS  = 2481;   // 21.5 ms: a first poll earlier than this still writes the note-on pitch

    struct Pattern
    {
        const int16_t* lead; int numLead;   // played once
        const int16_t* loop; int numLoop;   // repeated until key-off
    };

    // keys 21 to 44
    const int16_t kLowLead[] = { 2711 };
    const int16_t kLowLoop[] = { 3163, 451, -2255, -3157, -451, 2257 };
    // keys 45 to 71
    const int16_t kMidLead[] = { 460, 460, 0, -438, 0, 460, 460, 0, -438, 906 };
    const int16_t kMidLoop[] = { 1579, 672, -234, -1132, -1571, -234, 1132 };
    // keys 72 to 76
    const int16_t kUpperLead[] = { 441, 441, 0 };
    const int16_t kUpperLoop[] = { -441, 441, 1118, -235, -1354 };
    // keys 77 to 91
    const int16_t kHighLead[] = { 0 };
    const int16_t kHighLoop[] = { 905, 905, 0, -906, -906, 0 };

    const Pattern kPatterns[4] = {
        { kLowLead, 1, kLowLoop, 6 },   // keys 21 to 44
        { kMidLead, 10, kMidLoop, 7 },   // keys 45 to 71
        { kUpperLead, 3, kUpperLoop, 5 },   // keys 72 to 76
        { kHighLead, 0, kHighLoop, 6 }   // keys 77 to 91
    };

    // -1 = no vibrato (keys 92 to 108)
    inline int patternForNote(int note)
    {
        if (note <= 44) return 0;
        if (note <= 71) return 1;
        if (note <= 76) return 2;
        if (note <= 91) return 3;
        return -1;
    }

    struct State
    {
        int      pattern = -1;      // -1 = off
        int      index = 0;         // next entry of the sequence
        uint32_t base = 0;          // pitch at note-on
        uint32_t timer = 0;         // ticks until the next poll
        bool     skipFirst = false; // the first poll only writes the note-on pitch
    };

    // Call at note-on. envPollTicks = ticks until the first envelope poll of this voice.
    inline void start(State& s, int note, uint32_t pitch, uint32_t envPollTicks)
    {
        s.pattern = patternForNote(note);
        if (s.pattern < 0) return;

        s.base = pitch;
        s.index = 0;

        uint32_t first = envPollTicks + POLL_OFFSET_TICKS;
        if (first >= POLL_PERIOD_TICKS) first -= POLL_PERIOD_TICKS; // the poll before it falls on the note-on itself
        s.skipFirst = (first < EARLY_POLL_TICKS);
        s.timer = (first > 0) ? first : 1;
    }

    // Call once per engine tick. 'released' = the voice is in its key-off release.
    // 'pitch' is updated when the firmware would rewrite it.
    inline void tick(State& s, bool released, uint32_t& pitch)
    {
        if (s.pattern < 0 || released) return;

        if (s.timer > 0) --s.timer;
        if (s.timer != 0) return;
        s.timer = POLL_PERIOD_TICKS;

        if (s.skipFirst) { s.skipFirst = false; return; }

        const Pattern& p = kPatterns[s.pattern];
        const int i = s.index;
        const int ppm = (i < p.numLead) ? p.lead[i] : p.loop[(i - p.numLead) % p.numLoop];
        if (i < 1000000) ++s.index;

        const int64_t base = static_cast<int64_t>(s.base);
        pitch = static_cast<uint32_t>(base + base * static_cast<int64_t>(ppm) / 1000000);
    }
}
