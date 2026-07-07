#pragma once

#include <JuceHeader.h>

/**
    Master-bus effect rack, processed after voice summing in a fixed order:
    distortion -> bitcrusher -> phaser -> chorus -> delay -> reverb.

    Disabled effects are skipped entirely; each effect's state is reset when it
    is switched on so stale tails from minutes ago can't play back.
*/
class FXChain
{
public:
    struct Params
    {
        bool distOn = false;
        float distDrive = 0.3f, distMix = 1.0f;

        bool crushOn = false;
        int crushBits = 8, crushRate = 4;

        bool phaserOn = false;
        float phaserRate = 0.5f, phaserDepth = 0.7f, phaserMix = 0.5f;

        bool chorusOn = false;
        float chorusRate = 1.0f, chorusDepth = 0.3f, chorusMix = 0.5f;

        bool delayOn = false;
        float delayMs = 350.0f, delayFeedback = 0.4f, delayMix = 0.25f;

        bool reverbOn = false;
        float reverbSize = 0.6f, reverbDamp = 0.4f, reverbMix = 0.3f;

        bool pumpOn = false;
        float pumpAmount = 0.5f, pumpRate = 2.0f;
        bool pumpSync = false;
        juce::int64 blockStartSample = 0;
    };

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void setParams (const Params&);
    void process (juce::AudioBuffer<float>&);

private:
    void processDistortion (juce::AudioBuffer<float>&);
    void processBitcrusher (juce::AudioBuffer<float>&);
    void processDelay (juce::AudioBuffer<float>&);
    void processPump (juce::AudioBuffer<float>&);

    Params params;
    double sampleRate = 44100.0;
    int numChannels = 2;

    juce::dsp::Phaser<float> phaser;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    juce::SmoothedValue<float> delaySamplesSmoothed;
    juce::dsp::Reverb reverb;

    float crushHold[2] {};
    int crushCounter[2] {};
    float pumpPhase = 0.0f;
};
