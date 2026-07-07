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

    bool canPlaySound (juce::SynthesiserSound* sound) override;

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void prepare (double sampleRate, int samplesPerBlock, int numOutputChannels);
    void setParameters (const Wavetable* table, float position,
                        int unisonCount, float detuneCents, float spreadAmount,
                        float attack, float decay, float sustain, float release,
                        float cutoffHz, float resonance);

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

private:
    void updateUnison();

    WavetableOscillator oscs[maxUnison];
    float gainL[maxUnison] {}, gainR[maxUnison] {};

    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;
    juce::SmoothedValue<float> positionSmoothed;
    juce::Random random;

    juce::AudioBuffer<float> voiceBuffer;
    const Wavetable* table = nullptr;
    int numUnison = 1;
    float detune = 0.0f, spread = 0.0f;
    float noteHz = 0.0f;
    float level = 0.0f;
    float unisonNorm = 1.0f;
    bool isPrepared = false;
};
