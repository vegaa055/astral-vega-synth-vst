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
    cutoffParam     = apvts.getRawParameterValue ("filterCutoff");
    resonanceParam  = apvts.getRawParameterValue ("filterRes");
    gainParam       = apvts.getRawParameterValue ("gain");
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
        juce::ParameterID { "gain", 1 }, "Gain",
        juce::NormalisableRange<float> (-60.0f, 6.0f), -6.0f));

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

void AstralVegaAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();

    // Merge events from the on-screen keyboard into the incoming MIDI stream
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    const auto oscAParams = makeOscParams (oscARefs);
    const auto oscBParams = makeOscParams (oscBRefs);
    const int subOctavesDown = 1 + (int) subOctaveParam->load();

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
            voice->setParameters (oscAParams, oscBParams,
                                  subOctavesDown, subLevelParam->load(), noiseLevelParam->load(),
                                  attackParam->load(), decayParam->load(),
                                  sustainParam->load(), releaseParam->load(),
                                  cutoffParam->load(), resonanceParam->load());

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
