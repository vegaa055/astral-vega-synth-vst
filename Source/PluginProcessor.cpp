#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "TempoDivisions.h"

AstralVegaAudioProcessor::AstralVegaAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout()),
      wavetables (createFactoryWavetables())
{
    synth.addSound (new SynthSound());

    for (int i = 0; i < numVoices; ++i)
        synth.addVoice (new SynthVoice());

    wireOscParams (oscARefs, "oscA");
    wireOscParams (oscBRefs, "oscB");

    subOctaveParam  = apvts.getRawParameterValue ("subOctave");
    subLevelParam   = apvts.getRawParameterValue ("subLevel");
    noiseLevelParam = apvts.getRawParameterValue ("noiseLevel");
    attackParam     = apvts.getRawParameterValue ("attack");
    decayParam      = apvts.getRawParameterValue ("decay");
    sustainParam    = apvts.getRawParameterValue ("sustain");
    releaseParam    = apvts.getRawParameterValue ("release");
    cutoffParam         = apvts.getRawParameterValue ("filterCutoff");
    resonanceParam      = apvts.getRawParameterValue ("filterRes");
    filterMorphParam    = apvts.getRawParameterValue ("filterMorph");
    filterDriveParam    = apvts.getRawParameterValue ("filterDrive");
    filterKeytrackParam = apvts.getRawParameterValue ("filterKeytrack");
    filterEnvAmtParam   = apvts.getRawParameterValue ("filterEnvAmt");
    gainParam           = apvts.getRawParameterValue ("gain");
    voiceModeParam      = apvts.getRawParameterValue ("voiceMode");
    glideTimeParam      = apvts.getRawParameterValue ("glideTime");
    bendRangeParam      = apvts.getRawParameterValue ("pitchBendRange");

    lfo1ShapeParam   = apvts.getRawParameterValue ("lfo1Shape");
    lfo1RateParam    = apvts.getRawParameterValue ("lfo1Rate");
    lfo1SyncParam    = apvts.getRawParameterValue ("lfo1Sync");
    lfo1DivParam     = apvts.getRawParameterValue ("lfo1Div");
    lfo1FreeParam    = apvts.getRawParameterValue ("lfo1Free");
    lfo2ShapeParam   = apvts.getRawParameterValue ("lfo2Shape");
    lfo2RateParam    = apvts.getRawParameterValue ("lfo2Rate");
    lfo2SyncParam    = apvts.getRawParameterValue ("lfo2Sync");
    lfo2DivParam     = apvts.getRawParameterValue ("lfo2Div");
    lfo2FreeParam    = apvts.getRawParameterValue ("lfo2Free");
    pumpSyncParam    = apvts.getRawParameterValue ("pumpSync");
    pumpDivParam     = apvts.getRawParameterValue ("pumpDiv");
    delaySyncParam   = apvts.getRawParameterValue ("delaySync");
    delayDivParam    = apvts.getRawParameterValue ("delayDiv");
    env2AttackParam  = apvts.getRawParameterValue ("env2Attack");
    env2DecayParam   = apvts.getRawParameterValue ("env2Decay");
    env2SustainParam = apvts.getRawParameterValue ("env2Sustain");
    env2ReleaseParam = apvts.getRawParameterValue ("env2Release");

    for (int s = 0; s < SynthVoice::numModSlots; ++s)
    {
        const auto prefix = "mod" + juce::String (s + 1);
        modSrcParams[s] = apvts.getRawParameterValue (prefix + "Src");
        modDstParams[s] = apvts.getRawParameterValue (prefix + "Dst");
        modAmtParams[s] = apvts.getRawParameterValue (prefix + "Amt");
    }

    fxRefs.distOn        = apvts.getRawParameterValue ("distOn");
    fxRefs.distDrive     = apvts.getRawParameterValue ("distDrive");
    fxRefs.distMix       = apvts.getRawParameterValue ("distMix");
    fxRefs.crushOn       = apvts.getRawParameterValue ("crushOn");
    fxRefs.crushBits     = apvts.getRawParameterValue ("crushBits");
    fxRefs.crushRate     = apvts.getRawParameterValue ("crushRate");
    fxRefs.phaserOn      = apvts.getRawParameterValue ("phaserOn");
    fxRefs.phaserRate    = apvts.getRawParameterValue ("phaserRate");
    fxRefs.phaserDepth   = apvts.getRawParameterValue ("phaserDepth");
    fxRefs.phaserMix     = apvts.getRawParameterValue ("phaserMix");
    fxRefs.chorusOn      = apvts.getRawParameterValue ("chorusOn");
    fxRefs.chorusRate    = apvts.getRawParameterValue ("chorusRate");
    fxRefs.chorusDepth   = apvts.getRawParameterValue ("chorusDepth");
    fxRefs.chorusMix     = apvts.getRawParameterValue ("chorusMix");
    fxRefs.delayOn       = apvts.getRawParameterValue ("delayOn");
    fxRefs.delayTime     = apvts.getRawParameterValue ("delayTime");
    fxRefs.delayFeedback = apvts.getRawParameterValue ("delayFeedback");
    fxRefs.delayMix      = apvts.getRawParameterValue ("delayMix");
    fxRefs.reverbOn      = apvts.getRawParameterValue ("reverbOn");
    fxRefs.reverbSize    = apvts.getRawParameterValue ("reverbSize");
    fxRefs.reverbDamp    = apvts.getRawParameterValue ("reverbDamp");
    fxRefs.reverbMix     = apvts.getRawParameterValue ("reverbMix");
    fxRefs.pumpOn        = apvts.getRawParameterValue ("pumpOn");
    fxRefs.pumpAmount    = apvts.getRawParameterValue ("pumpAmount");
    fxRefs.pumpRate      = apvts.getRawParameterValue ("pumpRate");

    presetManager.onStateLoaded = [this]
    {
        {
            const juce::ScopedLock sl (statePathLock);
            pendingStatePath = apvts.state.getProperty ("userTablePath", {}).toString();
        }
        triggerAsyncUpdate();
    };
}

AstralVegaAudioProcessor::~AstralVegaAudioProcessor()
{
    cancelPendingUpdate();
    delete pendingUserTable.exchange (nullptr);
    delete retiredUserTable.exchange (nullptr);
}

juce::String AstralVegaAudioProcessor::loadUserWavetable (const juce::File& file)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));

    if (reader == nullptr)
        return "Couldn't open " + file.getFileName();

    constexpr int maxFrames = 256;
    const auto usableSamples = juce::jmin (reader->lengthInSamples,
                                           (juce::int64) Wavetable::frameSize * maxFrames);
    const int numFrames = (int) (usableSamples / Wavetable::frameSize);

    if (numFrames < 1)
        return file.getFileName() + " is shorter than one 2048-sample frame";

    juce::AudioBuffer<float> data (1, numFrames * Wavetable::frameSize);

    if (! reader->read (&data, 0, numFrames * Wavetable::frameSize, 0, true, false))
        return "Failed to read audio data from " + file.getFileName();

    auto newTable = std::make_unique<Wavetable> (file.getFileNameWithoutExtension(),
                                                 data.getReadPointer (0), numFrames);

    userDisplayBuffer.makeCopyOf (data);
    userDisplayFrames = numFrames;

    // clean up a table the audio thread retired earlier, then post the new one
    delete retiredUserTable.exchange (nullptr);
    delete pendingUserTable.exchange (newTable.release());

    currentUserTablePath = file.getFullPathName();
    apvts.state.setProperty ("userTablePath", currentUserTablePath, nullptr);

    return {};
}

AstralVegaAudioProcessor::DisplayTable
AstralVegaAudioProcessor::getDisplayTable (int tableIndex) const
{
    DisplayTable dt;

    if (juce::isPositiveAndBelow (tableIndex, (int) wavetables.size()))
    {
        const auto& table = wavetables[(size_t) tableIndex];
        dt.frames = table.frameData (0, 0);      // full-bandwidth mip
        dt.numFrames = table.numFrames;
        dt.frameStride = Wavetable::frameSize + 1;   // frames carry a guard sample
        dt.frameLength = Wavetable::frameSize;
    }
    else if (userDisplayFrames > 0)
    {
        dt.frames = userDisplayBuffer.getReadPointer (0);
        dt.numFrames = userDisplayFrames;
        dt.frameStride = Wavetable::frameSize;
        dt.frameLength = Wavetable::frameSize;
    }

    return dt;
}

void AstralVegaAudioProcessor::pushScopeSamples (const juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const auto* left = buffer.getReadPointer (0);
    const auto* right = buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : left;

    int start1, size1, start2, size2;
    scopeFifo.prepareToWrite (numSamples, start1, size1, start2, size2);

    for (int i = 0; i < size1; ++i)
        scopeStorage[(size_t) (start1 + i)] = 0.5f * (left[i] + right[i]);

    for (int i = 0; i < size2; ++i)
        scopeStorage[(size_t) (start2 + i)] = 0.5f * (left[size1 + i] + right[size1 + i]);

    scopeFifo.finishedWrite (size1 + size2);
}

int AstralVegaAudioProcessor::readScopeSamples (float* dest, int maxToRead)
{
    int start1, size1, start2, size2;
    scopeFifo.prepareToRead (maxToRead, start1, size1, start2, size2);

    for (int i = 0; i < size1; ++i)
        dest[i] = scopeStorage[(size_t) (start1 + i)];

    for (int i = 0; i < size2; ++i)
        dest[size1 + i] = scopeStorage[(size_t) (start2 + i)];

    scopeFifo.finishedRead (size1 + size2);
    return size1 + size2;
}

void AstralVegaAudioProcessor::handleAsyncUpdate()
{
    juce::String path;

    {
        const juce::ScopedLock sl (statePathLock);
        path = pendingStatePath;
    }

    if (path.isNotEmpty() && path != currentUserTablePath)
        loadUserWavetable (juce::File (path));
}

void AstralVegaAudioProcessor::wireOscParams (OscParamRefs& refs, const juce::String& idPrefix)
{
    refs.table  = apvts.getRawParameterValue (idPrefix + "Table");
    refs.pos    = apvts.getRawParameterValue (idPrefix + "Pos");
    refs.coarse = apvts.getRawParameterValue (idPrefix + "Coarse");
    refs.unison = apvts.getRawParameterValue (idPrefix + "Unison");
    refs.detune = apvts.getRawParameterValue (idPrefix + "Detune");
    refs.spread = apvts.getRawParameterValue (idPrefix + "Spread");
    refs.level  = apvts.getRawParameterValue (idPrefix + "Level");
}

juce::AudioProcessorValueTreeState::ParameterLayout AstralVegaAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const auto addOscSection = [&layout] (const juce::String& idPrefix, const juce::String& name,
                                          float defaultPos, float defaultLevel)
    {
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { idPrefix + "Table", 1 }, name + " Wavetable",
            juce::StringArray { "Basic", "PWM", "Spectra", "User" }, 0));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { idPrefix + "Pos", 1 }, name + " WT Position",
            juce::NormalisableRange<float> (0.0f, 1.0f), defaultPos));

        layout.add (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { idPrefix + "Coarse", 1 }, name + " Coarse", -24, 24, 0));

        layout.add (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { idPrefix + "Unison", 1 }, name + " Unison",
            1, SynthVoice::maxUnison, 1));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { idPrefix + "Detune", 1 }, name + " Detune",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.0f, 0.6f), 15.0f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { idPrefix + "Spread", 1 }, name + " Spread",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { idPrefix + "Level", 1 }, name + " Level",
            juce::NormalisableRange<float> (0.0f, 1.0f), defaultLevel));
    };

    addOscSection ("oscA", "Osc A", 0.66f, 0.8f);
    addOscSection ("oscB", "Osc B", 0.0f, 0.0f);

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "subOctave", 1 }, "Sub Octave",
        juce::StringArray { "-1 Oct", "-2 Oct" }, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "subLevel", 1 }, "Sub Level",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noiseLevel", 1 }, "Noise Level",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "attack", 1 }, "Attack",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.35f), 0.005f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "decay", 1 }, "Decay",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.35f), 0.2f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sustain", 1 }, "Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.8f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "release", 1 }, "Release",
        juce::NormalisableRange<float> (0.001f, 8.0f, 0.0f, 0.35f), 0.3f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterCutoff", 1 }, "Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.25f), 12000.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterRes", 1 }, "Resonance",
        juce::NormalisableRange<float> (0.5f, 8.0f, 0.0f, 0.5f), 0.707f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterMorph", 1 }, "Filter Morph",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterDrive", 1 }, "Filter Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterKeytrack", 1 }, "Filter Keytrack",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterEnvAmt", 1 }, "Filter Env Amount",
        juce::NormalisableRange<float> (-1.0f, 1.0f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gain", 1 }, "Gain",
        juce::NormalisableRange<float> (-60.0f, 6.0f), -6.0f));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "voiceMode", 1 }, "Voice Mode",
        juce::StringArray { "Poly", "Mono", "Legato" }, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "glideTime", 1 }, "Glide Time",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.0f, 0.4f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "pitchBendRange", 1 }, "Pitch Bend Range", 0, 24, 2));

    const juce::StringArray lfoShapes { "Sine", "Triangle", "Saw Down", "Square", "S&H" };

    const auto addLfo = [&layout, &lfoShapes] (const juce::String& idPrefix, const juce::String& name)
    {
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { idPrefix + "Shape", 1 }, name + " Shape", lfoShapes, 0));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { idPrefix + "Rate", 1 }, name + " Rate",
            juce::NormalisableRange<float> (0.02f, 20.0f, 0.0f, 0.4f), 2.0f));

        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { idPrefix + "Sync", 1 }, name + " Sync", false));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { idPrefix + "Div", 1 }, name + " Division",
            TempoDivisions::names(), TempoDivisions::quarterNote));

        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { idPrefix + "Free", 1 }, name + " Free Running", false));
    };

    addLfo ("lfo1", "LFO 1");
    addLfo ("lfo2", "LFO 2");

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "env2Attack", 1 }, "Env 2 Attack",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.35f), 0.01f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "env2Decay", 1 }, "Env 2 Decay",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.35f), 0.3f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "env2Sustain", 1 }, "Env 2 Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "env2Release", 1 }, "Env 2 Release",
        juce::NormalisableRange<float> (0.001f, 8.0f, 0.0f, 0.35f), 0.3f));

    const juce::StringArray modSources { "None", "LFO 1", "LFO 2", "Env 2", "Velocity", "Mod Wheel" };
    const juce::StringArray modTargets { "None", "Osc A Pos", "Osc B Pos", "Osc A Level",
                                         "Osc B Level", "Pitch", "Cutoff", "Resonance", "Morph" };

    for (int s = 1; s <= SynthVoice::numModSlots; ++s)
    {
        const auto prefix = "mod" + juce::String (s);
        const auto name = "Mod " + juce::String (s);

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { prefix + "Src", 1 }, name + " Source", modSources, 0));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { prefix + "Dst", 1 }, name + " Target", modTargets, 0));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { prefix + "Amt", 1 }, name + " Amount",
            juce::NormalisableRange<float> (-1.0f, 1.0f), 0.0f));
    }

    const auto addBool = [&layout] (const juce::String& id, const juce::String& name)
    {
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { id, 1 }, name, false));
    };

    const auto addFloat = [&layout] (const juce::String& id, const juce::String& name,
                                     juce::NormalisableRange<float> range, float def)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, def));
    };

    addBool ("distOn", "Dist On");
    addFloat ("distDrive", "Dist Drive", { 0.0f, 1.0f }, 0.3f);
    addFloat ("distMix", "Dist Mix", { 0.0f, 1.0f }, 1.0f);

    addBool ("crushOn", "Crush On");
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "crushBits", 1 }, "Crush Bits", 1, 16, 8));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "crushRate", 1 }, "Crush Rate", 1, 40, 4));

    addBool ("phaserOn", "Phaser On");
    addFloat ("phaserRate", "Phaser Rate", { 0.02f, 10.0f, 0.0f, 0.5f }, 0.5f);
    addFloat ("phaserDepth", "Phaser Depth", { 0.0f, 1.0f }, 0.7f);
    addFloat ("phaserMix", "Phaser Mix", { 0.0f, 1.0f }, 0.5f);

    addBool ("chorusOn", "Chorus On");
    addFloat ("chorusRate", "Chorus Rate", { 0.02f, 5.0f, 0.0f, 0.5f }, 1.0f);
    addFloat ("chorusDepth", "Chorus Depth", { 0.0f, 1.0f }, 0.3f);
    addFloat ("chorusMix", "Chorus Mix", { 0.0f, 1.0f }, 0.5f);

    addBool ("delayOn", "Delay On");
    addFloat ("delayTime", "Delay Time", { 10.0f, 1500.0f, 0.0f, 0.5f }, 350.0f);
    addFloat ("delayFeedback", "Delay Feedback", { 0.0f, 0.95f }, 0.4f);
    addFloat ("delayMix", "Delay Mix", { 0.0f, 1.0f }, 0.25f);

    addBool ("reverbOn", "Reverb On");
    addFloat ("reverbSize", "Reverb Size", { 0.0f, 1.0f }, 0.6f);
    addFloat ("reverbDamp", "Reverb Damping", { 0.0f, 1.0f }, 0.4f);
    addFloat ("reverbMix", "Reverb Mix", { 0.0f, 1.0f }, 0.3f);

    addBool ("pumpOn", "Pump On");
    addFloat ("pumpAmount", "Pump Amount", { 0.0f, 1.0f }, 0.5f);
    addFloat ("pumpRate", "Pump Rate", { 0.5f, 8.0f, 0.0f, 0.5f }, 2.0f);

    addBool ("pumpSync", "Pump Sync");
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "pumpDiv", 1 }, "Pump Division",
        TempoDivisions::names(), TempoDivisions::quarterNote));

    addBool ("delaySync", "Delay Sync");
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "delayDiv", 1 }, "Delay Division",
        TempoDivisions::names(), TempoDivisions::dottedEighth));

    return layout;
}

SynthVoice::OscParams AstralVegaAudioProcessor::makeOscParams (const OscParamRefs& refs) const
{
    SynthVoice::OscParams p;

    const auto tableIndex = juce::jlimit (0, (int) wavetables.size(),
                                          (int) refs.table->load());

    if (tableIndex >= (int) wavetables.size())   // the "User" slot
        p.table = activeUserTable != nullptr ? activeUserTable.get() : &wavetables[0];
    else
        p.table = &wavetables[(size_t) tableIndex];
    p.position = refs.pos->load();
    p.coarse = (int) refs.coarse->load();
    p.unisonCount = (int) refs.unison->load();
    p.detuneCents = refs.detune->load();
    p.spread = refs.spread->load();
    p.level = refs.level->load();

    return p;
}

void AstralVegaAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
            voice->prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    fxChain.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

bool AstralVegaAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

SynthVoice::BlockParams AstralVegaAudioProcessor::makeBlockParams() const
{
    SynthVoice::BlockParams bp;

    bp.oscA = makeOscParams (oscARefs);
    bp.oscB = makeOscParams (oscBRefs);

    bp.subOctavesDown = 1 + (int) subOctaveParam->load();
    bp.subLevel = subLevelParam->load();
    bp.noiseLevel = noiseLevelParam->load();

    bp.attack = attackParam->load();
    bp.decay = decayParam->load();
    bp.sustain = sustainParam->load();
    bp.release = releaseParam->load();

    bp.env2Attack = env2AttackParam->load();
    bp.env2Decay = env2DecayParam->load();
    bp.env2Sustain = env2SustainParam->load();
    bp.env2Release = env2ReleaseParam->load();

    bp.cutoffHz = cutoffParam->load();
    bp.resonance = resonanceParam->load();
    bp.filterMorph = filterMorphParam->load();
    bp.filterDrive = filterDriveParam->load();
    bp.filterKeytrack = filterKeytrackParam->load();
    bp.filterEnvAmt = filterEnvAmtParam->load();

    bp.voiceMode = (int) voiceModeParam->load();
    bp.glideTime = glideTimeParam->load();
    bp.pitchBendRange = (float) bendRangeParam->load();

    bp.lfo1Shape = (int) lfo1ShapeParam->load();
    bp.lfo1Rate = lfo1SyncParam->load() > 0.5f
                    ? TempoDivisions::toHz ((int) lfo1DivParam->load(), currentBpm)
                    : lfo1RateParam->load();
    bp.lfo1Global = lfo1FreeParam->load() > 0.5f;

    bp.lfo2Shape = (int) lfo2ShapeParam->load();
    bp.lfo2Rate = lfo2SyncParam->load() > 0.5f
                    ? TempoDivisions::toHz ((int) lfo2DivParam->load(), currentBpm)
                    : lfo2RateParam->load();
    bp.lfo2Global = lfo2FreeParam->load() > 0.5f;

    bp.blockStartSample = currentBlockStart;

    for (int s = 0; s < SynthVoice::numModSlots; ++s)
    {
        bp.routings[s].source = (int) modSrcParams[s]->load();
        bp.routings[s].target = (int) modDstParams[s]->load();
        bp.routings[s].amount = modAmtParams[s]->load();
    }

    bp.modWheel = modWheelValue;

    return bp;
}

FXChain::Params AstralVegaAudioProcessor::makeFXParams() const
{
    FXChain::Params fx;

    fx.distOn = fxRefs.distOn->load() > 0.5f;
    fx.distDrive = fxRefs.distDrive->load();
    fx.distMix = fxRefs.distMix->load();

    fx.crushOn = fxRefs.crushOn->load() > 0.5f;
    fx.crushBits = (int) fxRefs.crushBits->load();
    fx.crushRate = (int) fxRefs.crushRate->load();

    fx.phaserOn = fxRefs.phaserOn->load() > 0.5f;
    fx.phaserRate = fxRefs.phaserRate->load();
    fx.phaserDepth = fxRefs.phaserDepth->load();
    fx.phaserMix = fxRefs.phaserMix->load();

    fx.chorusOn = fxRefs.chorusOn->load() > 0.5f;
    fx.chorusRate = fxRefs.chorusRate->load();
    fx.chorusDepth = fxRefs.chorusDepth->load();
    fx.chorusMix = fxRefs.chorusMix->load();

    fx.delayOn = fxRefs.delayOn->load() > 0.5f;
    fx.delayMs = delaySyncParam->load() > 0.5f
                   ? juce::jlimit (10.0f, 1900.0f,
                                   TempoDivisions::toMs ((int) delayDivParam->load(), currentBpm))
                   : fxRefs.delayTime->load();
    fx.delayFeedback = fxRefs.delayFeedback->load();
    fx.delayMix = fxRefs.delayMix->load();

    fx.reverbOn = fxRefs.reverbOn->load() > 0.5f;
    fx.reverbSize = fxRefs.reverbSize->load();
    fx.reverbDamp = fxRefs.reverbDamp->load();
    fx.reverbMix = fxRefs.reverbMix->load();

    fx.pumpOn = fxRefs.pumpOn->load() > 0.5f;
    fx.pumpAmount = fxRefs.pumpAmount->load();
    fx.pumpSync = pumpSyncParam->load() > 0.5f;
    fx.pumpRate = fx.pumpSync
                    ? TempoDivisions::toHz ((int) pumpDivParam->load(), currentBpm)
                    : fxRefs.pumpRate->load();
    fx.blockStartSample = currentBlockStart;

    return fx;
}

void AstralVegaAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();

    // adopt a freshly imported user wavetable, retiring the old one for the
    // message thread to delete (voices only hold table pointers per block)
    if (auto* incoming = pendingUserTable.exchange (nullptr))
    {
        if (activeUserTable != nullptr)
            delete retiredUserTable.exchange (activeUserTable.release());

        activeUserTable.reset (incoming);
    }

    // host tempo + timeline position; fall back to 120 BPM / an internal
    // counter when no playhead is available (e.g. standalone)
    double bpm = 120.0;
    juce::int64 blockStart = internalSampleCount;

    if (auto* playhead = getPlayHead())
    {
        if (const auto position = playhead->getPosition())
        {
            if (const auto hostBpm = position->getBpm())
                bpm = *hostBpm;

            if (const auto timeSamples = position->getTimeInSamples())
                blockStart = *timeSamples;
        }
    }

    currentBpm = bpm;
    currentBlockStart = blockStart;
    internalSampleCount += buffer.getNumSamples();

    synth.setVoiceMode ((int) voiceModeParam->load());

    // Merge events from the on-screen keyboard into the incoming MIDI stream
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();

        if (msg.isController() && msg.getControllerNumber() == 1)
            modWheelValue = (float) msg.getControllerValue() / 127.0f;
    }

    const auto blockParams = makeBlockParams();

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
            voice->setParameters (blockParams);

    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    fxChain.setParams (makeFXParams());
    fxChain.process (buffer);

    buffer.applyGain (juce::Decibels::decibelsToGain (gainParam->load()));

    pushScopeSamples (buffer);
}

juce::AudioProcessorEditor* AstralVegaAudioProcessor::createEditor()
{
    return new AstralVegaAudioProcessorEditor (*this);
}

void AstralVegaAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AstralVegaAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));

    // restore the user wavetable on the message thread
    {
        const juce::ScopedLock sl (statePathLock);
        pendingStatePath = apvts.state.getProperty ("userTablePath", {}).toString();
    }
    triggerAsyncUpdate();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AstralVegaAudioProcessor();
}
