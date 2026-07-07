#pragma once

#include <JuceHeader.h>
#include "Wavetable.h"
#include "LFO.h"
#include "MorphFilter.h"

struct SynthSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class SynthVoice : public juce::SynthesiserVoice
{
public:
    static constexpr int maxUnison = 7;
    static constexpr int numModSlots = 6;

    enum ModSourceIndex
    {
        srcNone = 0, srcLFO1, srcLFO2, srcEnv2, srcVelocity, srcModWheel,
        numModSources
    };

    enum ModTargetIndex
    {
        tgtNone = 0, tgtOscAPos, tgtOscBPos, tgtOscALevel, tgtOscBLevel,
        tgtPitch, tgtCutoff, tgtResonance, tgtFilterMorph,
        numModTargets
    };

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

    struct ModRouting
    {
        int source = srcNone;
        int target = tgtNone;
        float amount = 0.0f;
    };

    /** Everything the voice needs for one block, built once by the processor. */
    struct BlockParams
    {
        OscParams oscA, oscB;
        int subOctavesDown = 1;
        float subLevel = 0.0f, noiseLevel = 0.0f;
        float attack = 0.005f, decay = 0.2f, sustain = 0.8f, release = 0.3f;
        float env2Attack = 0.01f, env2Decay = 0.3f, env2Sustain = 0.5f, env2Release = 0.3f;
        float cutoffHz = 12000.0f, resonance = 0.707f;
        float filterMorph = 0.0f, filterDrive = 0.0f;
        float filterKeytrack = 0.0f, filterEnvAmt = 0.0f;
        int lfo1Shape = 0; float lfo1Rate = 2.0f;
        int lfo2Shape = 0; float lfo2Rate = 2.0f;
        ModRouting routings[numModSlots];
        float modWheel = 0.0f;
    };

    bool canPlaySound (juce::SynthesiserSound* sound) override;

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void prepare (double sampleRate, int samplesPerBlock, int numOutputChannels);
    void setParameters (const BlockParams&);

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

private:
    static constexpr int controlInterval = 32;   // samples between modulation updates

    /** A wavetable oscillator with unison — the voice owns two of these (A and B). */
    struct OscSection
    {
        void reset (double sampleRate);
        void applyParams (const OscParams& p, float effectiveHz, double sampleRate);
        void noteStart (const OscParams& p, float effectiveHz, double sampleRate, juce::Random& rng);
        void updateFrequencies (float effectiveHz, double sampleRate);
        void updatePanGains();
        void updateTuning (float effectiveHz, double sampleRate);

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

    void updateModulation (int chunkSamples);
    void updateSubInc();

    OscSection oscA, oscB;
    LFO lfo1, lfo2;

    juce::SmoothedValue<float> subLevelSmoothed, noiseLevelSmoothed;
    float subPhase = 0.0f, subInc = 0.0f;
    int subOctave = 1;             // octaves below the played note

    MorphFilter filter;
    juce::ADSR adsr, env2;
    juce::ADSR::Parameters adsrParams, env2Params;
    juce::Random random;

    juce::AudioBuffer<float> voiceBuffer;
    BlockParams params;

    float noteHz = 0.0f;
    int noteNumber = 60;
    float pitchMul = 1.0f;
    float currentPitchSemis = 0.0f;
    float velocity01 = 0.0f;
    float level = 0.0f;
    bool isPrepared = false;
};
