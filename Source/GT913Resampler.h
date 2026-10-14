/*
  ==============================================================================
    GT913Resampler.h
    Band-limited sample rate converter (windowed-sinc) for the GT913 engine.
    Converts the engine's fixed output rate (about 114.5 kHz) to the host rate.
    All comments are 100% English ASCII.
  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

class GT913Resampler
{
public:
    // Number of source samples used on each side of the interpolation point.
    // 56 gives a pass band that is flat up to about 21 kHz at 48 kHz output,
    // and strong rejection above about 27 kHz.
    static constexpr int HALF_TAPS = 56;
    static constexpr int TABLE_RES = 256; // kernel table entries per source sample

    void prepare(double srcRate, double dstRate)
    {
        ratio = srcRate / dstRate;
        // cut off at the lower of the two Nyquist frequencies (cycles per source sample)
        const double fc = 0.5 * std::min(1.0, dstRate / srcRate);
        buildKernel(fc);
        reset();
    }

    void reset()
    {
        std::fill(bufL.begin(), bufL.end(), 0.0f);
        std::fill(bufR.begin(), bufR.end(), 0.0f);
        pushed = 0;
        outPos = 0.0;
    }

    // True while more source frames are needed before the next output sample can be produced.
    bool needsInput() const
    {
        return pushed <= static_cast<int64_t>(std::floor(outPos)) + HALF_TAPS;
    }

    // Add one source-rate stereo frame.
    void push(float left, float right)
    {
        bufL[static_cast<size_t>(pushed & MASK)] = left;
        bufR[static_cast<size_t>(pushed & MASK)] = right;
        ++pushed;
    }

    // Produce one output frame. Call only when needsInput() is false.
    void pop(float& left, float& right)
    {
        const int64_t k0 = static_cast<int64_t>(std::floor(outPos));
        double sumL = 0.0, sumR = 0.0, sumW = 0.0;

        for (int k = -HALF_TAPS + 1; k <= HALF_TAPS; ++k)
        {
            const int64_t idx = k0 + k;
            const double w = kernelAt(std::fabs(outPos - static_cast<double>(idx)));
            sumW += w;
            if (idx >= 0)
            {
                sumL += w * bufL[static_cast<size_t>(idx & MASK)];
                sumR += w * bufR[static_cast<size_t>(idx & MASK)];
            }
        }

        const double norm = (sumW != 0.0) ? 1.0 / sumW : 0.0; // keeps the DC gain at exactly 1
        left  = static_cast<float>(sumL * norm);
        right = static_cast<float>(sumR * norm);

        outPos += ratio;

        // keep the counters small (REBASE is a multiple of the ring buffer size)
        if (pushed > REBASE)
        {
            pushed -= REBASE;
            outPos -= static_cast<double>(REBASE);
        }
    }

private:
    static constexpr int64_t BUF_SIZE = 512;
    static constexpr int64_t MASK = BUF_SIZE - 1;
    static constexpr int64_t REBASE = int64_t(1) << 30;

    double  ratio = 1.0;
    double  outPos = 0.0;   // next output time, in source samples
    int64_t pushed = 0;     // number of source frames pushed so far
    std::vector<float>  bufL = std::vector<float>(BUF_SIZE, 0.0f);
    std::vector<float>  bufR = std::vector<float>(BUF_SIZE, 0.0f);
    std::vector<double> table;

    static double besselI0(double x)
    {
        double sum = 1.0, term = 1.0;
        const double q = x * x / 4.0;
        for (int k = 1; k < 60; ++k)
        {
            term *= q / (static_cast<double>(k) * k);
            sum += term;
            if (term < sum * 1e-16) break;
        }
        return sum;
    }

    void buildKernel(double fc)
    {
        const double beta = 8.0; // Kaiser window, about 80 dB stop band
        const double pi = 3.14159265358979323846;
        const int n = HALF_TAPS * TABLE_RES + 2;
        table.assign(static_cast<size_t>(n), 0.0);
        const double i0beta = besselI0(beta);
        for (int j = 0; j < n; ++j)
        {
            const double x = static_cast<double>(j) / TABLE_RES;
            if (x >= HALF_TAPS) continue;
            const double y = 2.0 * fc * x;
            const double s = (y == 0.0) ? 1.0 : std::sin(pi * y) / (pi * y);
            const double u = x / HALF_TAPS;
            const double win = besselI0(beta * std::sqrt(std::max(0.0, 1.0 - u * u))) / i0beta;
            table[static_cast<size_t>(j)] = 2.0 * fc * s * win;
        }
    }

    double kernelAt(double ax) const
    {
        const double pos = ax * TABLE_RES;
        const size_t i = static_cast<size_t>(pos);
        if (i + 1 >= table.size()) return 0.0;
        const double frac = pos - static_cast<double>(i);
        return table[i] + frac * (table[i + 1] - table[i]);
    }
};
