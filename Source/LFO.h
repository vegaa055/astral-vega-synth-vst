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
        const float value = valueAtPhase();

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

private:
    float valueAtPhase() const
    {
        switch (shape)
        {
            case sine:       return std::sin (juce::MathConstants<float>::twoPi * phase);
            case triangle:   return 4.0f * std::abs (phase - 0.5f) - 1.0f;
            case sawDown:    return 1.0f - 2.0f * phase;
            case square:     return phase < 0.5f ? 1.0f : -1.0f;
            case sampleHold: return shValue;
            default:         break;
        }

        return 0.0f;
    }

    double sampleRate = 0.0;
    int shape = sine;
    float rateHz = 1.0f;
    float phase = 0.0f;
    float shValue = 0.0f;
};
