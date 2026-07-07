#pragma once

#include <JuceHeader.h>
#include "Wavetable.h"

struct SynthSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class SynthVoice : public juce::SynthesiserVoice
{
public:
    static constexpr int maxUnison = 7;

    /** One wavetable oscillator section's parameter snapshot for a block. */
    struct OscParams
    {
        const Wavetable* table = nullptr;
        float position = 0.0f;
        int coarse = 0;            // semitones
        int unisonCount = 1;
        float detuneCents = 0.0f;
        float spread = 0.0f;
        float level = 0.0f;
    };

    bool canPlaySound (juce::SynthesiserSound* sound) override;

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void prepare (double sampleRate, int samplesPerBlock, int numOutputChannels);
    void setParameters (const OscParams& oscAParams, const OscParams& oscBParams,
                        int subOctavesDown, float subLevel, float noiseLevel,
                        float attack, float decay, float sustain, float release,
                        float cutoffHz, float resonance);

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

private:
    /** A wavetable oscillator with unison — the voice owns two of these (A and B). */
    struct OscSection
    {
        void reset (double sampleRate);
        void applyParams (const OscParams& p, float noteHz, double sampleRate);
        void noteStart (float noteHz, double sampleRate, juce::Random& rng);
        void updateTuning (float noteHz, double sampleRate);

        forcedinline void renderSample (float& outL, float& outR) noexcept
        {
            const float lev = level.getNextValue();
            const float pos = position.getNextValue();

            if (table == nullptr || lev <= 0.0001f)
                return;

            const float framePos = pos * (float) (table->numFrames - 1);
            float l = 0.0f, r = 0.0f;

            for (int u = 0; u < numUnison; ++u)
            {
                const float s = oscs[u].getNextSample (framePos);
                l += s * gainL[u];
                r += s * gainR[u];
            }

            const float g = lev * unisonNorm;
            outL += l * g;
            outR += r * g;
        }

        WavetableOscillator oscs[maxUnison];
        float gainL[maxUnison] {}, gainR[maxUnison] {};
        juce::SmoothedValue<float> position, level;
        const Wavetable* table = nullptr;
        int numUnison = 1, coarse = 0;
        float detune = 0.0f, spread = 0.0f;
        float unisonNorm = 1.0f;
    };

    void updateSubInc();

    OscSection oscA, oscB;

    juce::SmoothedValue<float> subLevelSmoothed, noiseLevelSmoothed;
    float subPhase = 0.0f, subInc = 0.0f;
    int subOctave = 1;             // octaves below the played note

    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;
    juce::Random random;

    juce::AudioBuffer<float> voiceBuffer;
    float noteHz = 0.0f;
    float level = 0.0f;
    bool isPrepared = false;
};
