#pragma once

#include <JuceHeader.h>

/**
    Stereo state-variable filter (topology-preserving transform, after
    Zavalishin / Simper) with a morphable output — one knob sweeps
    low-pass -> band-pass -> high-pass by crossfading the three outputs the
    core computes simultaneously — plus an optional tanh drive stage on the
    input for analogue-style saturation into the filter.

    Coefficients are updated at control rate via setParams(); processBlock()
    then runs a chunk of samples with fixed coefficients.
*/
class MorphFilter
{
public:
    void prepare (double sampleRateIn, int numChannelsIn)
    {
        sampleRate = sampleRateIn;
        numChannels = juce::jlimit (1, maxChannels, numChannelsIn);
        reset();
    }

    void reset()
    {
        for (auto& s : state)
            s = {};
    }

    void setParams (float cutoffHz, float q, float morph01, float driveAmount)
    {
        const float limit = (float) (sampleRate * 0.49);
        const float g = std::tan (juce::MathConstants<float>::pi
                                  * juce::jlimit (20.0f, limit, cutoffHz) / (float) sampleRate);

        k = 1.0f / juce::jmax (0.1f, q);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;

        const float morph = juce::jlimit (0.0f, 1.0f, morph01);

        if (morph < 0.5f)
        {
            lpMix = 1.0f - 2.0f * morph;
            bpMix = 2.0f * morph;
            hpMix = 0.0f;
        }
        else
        {
            lpMix = 0.0f;
            bpMix = 2.0f - 2.0f * morph;
            hpMix = 2.0f * morph - 1.0f;
        }

        driveActive = driveAmount > 0.001f;
        driveGain = 1.0f + driveAmount * 15.0f;
        driveMakeup = 1.0f / std::sqrt (driveGain);
    }

    void processBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
    {
        const int channels = juce::jmin (buffer.getNumChannels(), numChannels);

        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            auto& s = state[ch];

            for (int i = startSample; i < startSample + numSamples; ++i)
            {
                float x = data[i];

                if (driveActive)
                    x = std::tanh (x * driveGain) * driveMakeup;

                const float v3 = x - s.ic2;
                const float v1 = a1 * s.ic1 + a2 * v3;
                const float v2 = s.ic2 + a2 * s.ic1 + a3 * v3;
                s.ic1 = 2.0f * v1 - s.ic1;
                s.ic2 = 2.0f * v2 - s.ic2;

                const float lp = v2;
                const float bp = v1 * k;   // normalised to unity peak gain
                const float hp = x - k * v1 - v2;

                data[i] = lp * lpMix + bp * bpMix + hp * hpMix;
            }
        }
    }

private:
    static constexpr int maxChannels = 2;

    struct State { float ic1 = 0.0f, ic2 = 0.0f; };
    State state[maxChannels];

    double sampleRate = 44100.0;
    int numChannels = 2;

    float k = 1.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float lpMix = 1.0f, bpMix = 0.0f, hpMix = 0.0f;
    bool driveActive = false;
    float driveGain = 1.0f, driveMakeup = 1.0f;
};
