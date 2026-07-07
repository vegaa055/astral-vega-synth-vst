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

    oscTableParam  = apvts.getRawParameterValue ("oscTable");
    oscPosParam    = apvts.getRawParameterValue ("oscPos");
    unisonParam    = apvts.getRawParameterValue ("unison");
    detuneParam    = apvts.getRawParameterValue ("detune");
    spreadParam    = apvts.getRawParameterValue ("spread");
    attackParam    = apvts.getRawParameterValue ("attack");
    decayParam     = apvts.getRawParameterValue ("decay");
    sustainParam   = apvts.getRawParameterValue ("sustain");
    releaseParam   = apvts.getRawParameterValue ("release");
    cutoffParam    = apvts.getRawParameterValue ("filterCutoff");
    resonanceParam = apvts.getRawParameterValue ("filterRes");
    gainParam      = apvts.getRawParameterValue ("gain");
}

juce::AudioProcessorValueTreeState::ParameterLayout AstralVegaAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "oscTable", 1 }, "Wavetable",
        juce::StringArray { "Basic", "PWM", "Spectra" }, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "oscPos", 1 }, "WT Position",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.66f));

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "unison", 1 }, "Unison", 1, SynthVoice::maxUnison, 1));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "detune", 1 }, "Detune",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.0f, 0.6f), 15.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "spread", 1 }, "Spread",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

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

    const auto tableIndex = juce::jlimit (0, (int) wavetables.size() - 1,
                                          (int) oscTableParam->load());
    const auto* table = &wavetables[(size_t) tableIndex];

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
            voice->setParameters (table, oscPosParam->load(),
                                  (int) unisonParam->load(),
                                  detuneParam->load(), spreadParam->load(),
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
