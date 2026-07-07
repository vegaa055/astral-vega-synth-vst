#include "PluginProcessor.h"
#include "PluginEditor.h"

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

    lfo1ShapeParam   = apvts.getRawParameterValue ("lfo1Shape");
    lfo1RateParam    = apvts.getRawParameterValue ("lfo1Rate");
    lfo2ShapeParam   = apvts.getRawParameterValue ("lfo2Shape");
    lfo2RateParam    = apvts.getRawParameterValue ("lfo2Rate");
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
            juce::StringArray { "Basic", "PWM", "Spectra" }, 0));

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

    const juce::StringArray lfoShapes { "Sine", "Triangle", "Saw Down", "Square", "S&H" };

    const auto addLfo = [&layout, &lfoShapes] (const juce::String& idPrefix, const juce::String& name)
    {
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { idPrefix + "Shape", 1 }, name + " Shape", lfoShapes, 0));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { idPrefix + "Rate", 1 }, name + " Rate",
            juce::NormalisableRange<float> (0.02f, 20.0f, 0.0f, 0.4f), 2.0f));
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

    return layout;
}

SynthVoice::OscParams AstralVegaAudioProcessor::makeOscParams (const OscParamRefs& refs) const
{
    SynthVoice::OscParams p;

    const auto tableIndex = juce::jlimit (0, (int) wavetables.size() - 1,
                                          (int) refs.table->load());
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

    bp.lfo1Shape = (int) lfo1ShapeParam->load();
    bp.lfo1Rate = lfo1RateParam->load();
    bp.lfo2Shape = (int) lfo2ShapeParam->load();
    bp.lfo2Rate = lfo2RateParam->load();

    for (int s = 0; s < SynthVoice::numModSlots; ++s)
    {
        bp.routings[s].source = (int) modSrcParams[s]->load();
        bp.routings[s].target = (int) modDstParams[s]->load();
        bp.routings[s].amount = modAmtParams[s]->load();
    }

    bp.modWheel = modWheelValue;

    return bp;
}

void AstralVegaAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();

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

    buffer.applyGain (juce::Decibels::decibelsToGain (gainParam->load()));
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
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AstralVegaAudioProcessor();
}
