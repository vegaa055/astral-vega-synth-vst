#pragma once

#include <JuceHeader.h>

/** Per-voice LFO, evaluated at control rate (once per chunk), retriggered on note-on. */
class LFO
{
public:
    enum Shape { sine = 0, triangle, sawDown, square, sampleHold };

    void prepare (double sampleRateIn) { sampleRate = sampleRateIn; }

    void setParams (int shapeIn, float rateHzIn)
    {
        shape = shapeIn;
        rateHz = rateHzIn;
    }

    void noteOn (juce::Random& rng)
    {
        phase = 0.0f;
        shValue = rng.nextFloat() * 2.0f - 1.0f;
    }

    /** Returns the value (-1..1) at the current phase, then advances by numSamples. */
    float tick (int numSamples, juce::Random& rng)
    {
        const float value = valueFor (phase);

        if (sampleRate > 0.0)
        {
            phase += (float) (rateHz * numSamples / sampleRate);

            while (phase >= 1.0f)
            {
                phase -= 1.0f;
                shValue = rng.nextFloat() * 2.0f - 1.0f;
            }
        }

        return value;
    }

    /** Free-running value derived from the absolute timeline position — every
        voice computes the same value, and renders line up with the host grid. */
    float globalValue (juce::int64 absoluteSample) const
    {
        if (sampleRate <= 0.0)
            return 0.0f;

        const double cycles = (double) absoluteSample * rateHz / sampleRate;

        if (shape == sampleHold)
            return hashToBipolar ((juce::int64) std::floor (cycles));

        return valueFor ((float) (cycles - std::floor (cycles)));
    }

private:
    float valueFor (float ph) const
    {
        switch (shape)
        {
            case sine:       return std::sin (juce::MathConstants<float>::twoPi * ph);
            case triangle:   return 4.0f * std::abs (ph - 0.5f) - 1.0f;
            case sawDown:    return 1.0f - 2.0f * ph;
            case square:     return ph < 0.5f ? 1.0f : -1.0f;
            case sampleHold: return shValue;
            default:         break;
        }

        return 0.0f;
    }

    /** Deterministic pseudo-random value per S&H cycle, identical across voices. */
    static float hashToBipolar (juce::int64 n)
    {
        auto x = (juce::uint64) n * 0x9E3779B97F4A7C15ULL;
        x ^= x >> 33;
        x *= 0xC2B2AE3D27D4EB4FULL;
        x ^= x >> 29;
        return (float) (x & 0xFFFFFF) / (float) 0x7FFFFF - 1.0f;
    }

    double sampleRate = 0.0;
    int shape = sine;
    float rateHz = 1.0f;
    float phase = 0.0f;
    float shValue = 0.0f;
};
