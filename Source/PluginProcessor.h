#pragma once

#include <JuceHeader.h>
#include "SynthVoice.h"
#include "AstralSynth.h"
#include "FXChain.h"
#include "PresetManager.h"

class AstralVegaAudioProcessor : public juce::AudioProcessor,
                                 private juce::AsyncUpdater
{
public:
    AstralVegaAudioProcessor();
    ~AstralVegaAudioProcessor() override;

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
    PresetManager presetManager { apvts };

    /** Imports a .wav of concatenated 2048-sample frames into the "User"
        wavetable slot. Message thread only. Returns an error string, or {}. */
    juce::String loadUserWavetable (const juce::File&);

    /** Pulls mono post-FX samples for the editor's scope/spectrum display. */
    int readScopeSamples (float* dest, int maxToRead);

    /** Read-only view of a wavetable's frames for the editor's display.

        Factory tables are immutable for the processor's lifetime, and the
        user table's display copy is owned by the message thread, so this
        never races with the audio thread's table swapping. Message thread only.
    */
    struct DisplayTable
    {
        const float* frames = nullptr;
        int numFrames = 0;
        int frameStride = 0;    // samples between frame starts
        int frameLength = 0;    // usable samples per frame
        int generation = 0;     // bumps when the user table is replaced

        bool isValid() const noexcept { return frames != nullptr && numFrames > 0; }
    };

    DisplayTable getDisplayTable (int tableIndex) const;

    /** Wavetable position of the newest sounding voice, with modulation
        applied — what you are actually hearing right now. Returns false when
        no voice is active, in which case the display should fall back to the
        parameter value. Any thread. */
    bool getLivePosition (int oscIndex, float& positionOut) const;

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

    struct FXParamRefs
    {
        std::atomic<float>* distOn = nullptr;
        std::atomic<float>* distDrive = nullptr;
        std::atomic<float>* distMix = nullptr;
        std::atomic<float>* crushOn = nullptr;
        std::atomic<float>* crushBits = nullptr;
        std::atomic<float>* crushRate = nullptr;
        std::atomic<float>* phaserOn = nullptr;
        std::atomic<float>* phaserRate = nullptr;
        std::atomic<float>* phaserDepth = nullptr;
        std::atomic<float>* phaserMix = nullptr;
        std::atomic<float>* chorusOn = nullptr;
        std::atomic<float>* chorusRate = nullptr;
        std::atomic<float>* chorusDepth = nullptr;
        std::atomic<float>* chorusMix = nullptr;
        std::atomic<float>* delayOn = nullptr;
        std::atomic<float>* delayTime = nullptr;
        std::atomic<float>* delayFeedback = nullptr;
        std::atomic<float>* delayMix = nullptr;
        std::atomic<float>* reverbOn = nullptr;
        std::atomic<float>* reverbSize = nullptr;
        std::atomic<float>* reverbDamp = nullptr;
        std::atomic<float>* reverbMix = nullptr;
        std::atomic<float>* pumpOn = nullptr;
        std::atomic<float>* pumpAmount = nullptr;
        std::atomic<float>* pumpRate = nullptr;
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void wireOscParams (OscParamRefs&, const juce::String& idPrefix);
    SynthVoice::OscParams makeOscParams (const OscParamRefs&) const;
    SynthVoice::BlockParams makeBlockParams() const;
    FXChain::Params makeFXParams() const;

    std::vector<Wavetable> wavetables;
    AstralSynth synth;
    FXChain fxChain;

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
    std::atomic<float>* filterMorphParam = nullptr;
    std::atomic<float>* filterDriveParam = nullptr;
    std::atomic<float>* filterKeytrackParam = nullptr;
    std::atomic<float>* filterEnvAmtParam = nullptr;
    std::atomic<float>* gainParam = nullptr;
    std::atomic<float>* voiceModeParam = nullptr;
    std::atomic<float>* glideTimeParam = nullptr;
    std::atomic<float>* bendRangeParam = nullptr;

    std::atomic<float>* lfo1ShapeParam = nullptr;
    std::atomic<float>* lfo1RateParam = nullptr;
    std::atomic<float>* lfo1SyncParam = nullptr;
    std::atomic<float>* lfo1DivParam = nullptr;
    std::atomic<float>* lfo1FreeParam = nullptr;
    std::atomic<float>* lfo2ShapeParam = nullptr;
    std::atomic<float>* lfo2RateParam = nullptr;
    std::atomic<float>* lfo2SyncParam = nullptr;
    std::atomic<float>* lfo2DivParam = nullptr;
    std::atomic<float>* lfo2FreeParam = nullptr;
    std::atomic<float>* pumpSyncParam = nullptr;
    std::atomic<float>* pumpDivParam = nullptr;
    std::atomic<float>* delaySyncParam = nullptr;
    std::atomic<float>* delayDivParam = nullptr;
    std::atomic<float>* env2AttackParam = nullptr;
    std::atomic<float>* env2DecayParam = nullptr;
    std::atomic<float>* env2SustainParam = nullptr;
    std::atomic<float>* env2ReleaseParam = nullptr;

    std::atomic<float>* modSrcParams[SynthVoice::numModSlots] {};
    std::atomic<float>* modDstParams[SynthVoice::numModSlots] {};
    std::atomic<float>* modAmtParams[SynthVoice::numModSlots] {};

    void handleAsyncUpdate() override;

    FXParamRefs fxRefs;

    float modWheelValue = 0.0f;
    double currentBpm = 120.0;
    juce::int64 currentBlockStart = 0;
    juce::int64 internalSampleCount = 0;

    // user wavetable hand-off: message thread builds and posts via pending;
    // the audio thread adopts it between blocks and retires the old table
    // back to the message thread for deletion
    std::atomic<Wavetable*> pendingUserTable { nullptr };
    std::atomic<Wavetable*> retiredUserTable { nullptr };
    std::unique_ptr<Wavetable> activeUserTable;   // audio thread only

    juce::CriticalSection statePathLock;
    juce::String pendingStatePath;                // guarded by statePathLock
    juce::String currentUserTablePath;            // message thread only

    // raw imported frames kept for the editor's display, so it never has to
    // read the audio thread's live table (message thread only)
    juce::AudioBuffer<float> userDisplayBuffer;
    int userDisplayFrames = 0;
    int userTableGeneration = 0;

    void publishLivePositions();

    std::atomic<float> livePosition[2] {};
    std::atomic<bool> liveVoiceActive { false };

    void pushScopeSamples (const juce::AudioBuffer<float>&);

    juce::AbstractFifo scopeFifo { 8192 };
    std::vector<float> scopeStorage = std::vector<float> (8192, 0.0f);

    static constexpr int numVoices = 16;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AstralVegaAudioProcessor)
};
