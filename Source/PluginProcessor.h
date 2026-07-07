#pragma once

#include <JuceHeader.h>
#include "SynthVoice.h"

class AstralVegaAudioProcessor : public juce::AudioProcessor
{
public:
    AstralVegaAudioProcessor();
    ~AstralVegaAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

private:
    struct OscParamRefs
    {
        std::atomic<float>* table = nullptr;
        std::atomic<float>* pos = nullptr;
        std::atomic<float>* coarse = nullptr;
        std::atomic<float>* unison = nullptr;
        std::atomic<float>* detune = nullptr;
        std::atomic<float>* spread = nullptr;
        std::atomic<float>* level = nullptr;
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void wireOscParams (OscParamRefs&, const juce::String& idPrefix);
    SynthVoice::OscParams makeOscParams (const OscParamRefs&) const;
    SynthVoice::BlockParams makeBlockParams() const;

    std::vector<Wavetable> wavetables;
    juce::Synthesiser synth;

    OscParamRefs oscARefs, oscBRefs;

    std::atomic<float>* subOctaveParam = nullptr;
    std::atomic<float>* subLevelParam = nullptr;
    std::atomic<float>* noiseLevelParam = nullptr;
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* sustainParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;
    std::atomic<float>* cutoffParam = nullptr;
    std::atomic<float>* resonanceParam = nullptr;
    std::atomic<float>* gainParam = nullptr;

    std::atomic<float>* lfo1ShapeParam = nullptr;
    std::atomic<float>* lfo1RateParam = nullptr;
    std::atomic<float>* lfo2ShapeParam = nullptr;
    std::atomic<float>* lfo2RateParam = nullptr;
    std::atomic<float>* env2AttackParam = nullptr;
    std::atomic<float>* env2DecayParam = nullptr;
    std::atomic<float>* env2SustainParam = nullptr;
    std::atomic<float>* env2ReleaseParam = nullptr;

    std::atomic<float>* modSrcParams[SynthVoice::numModSlots] {};
    std::atomic<float>* modDstParams[SynthVoice::numModSlots] {};
    std::atomic<float>* modAmtParams[SynthVoice::numModSlots] {};

    float modWheelValue = 0.0f;

    static constexpr int numVoices = 16;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AstralVegaAudioProcessor)
};
