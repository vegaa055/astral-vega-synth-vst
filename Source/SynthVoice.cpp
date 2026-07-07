#include "SynthVoice.h"

//==============================================================================
// OscSection

void SynthVoice::OscSection::reset (double sampleRate)
{
    position.reset (sampleRate, 0.05);
    level.reset (sampleRate, 0.02);
}

void SynthVoice::OscSection::applyParams (const OscParams& p, float noteHz, double sampleRate)
{
    if (p.table != table)
    {
        table = p.table;
        for (auto& osc : oscs)
            osc.setTable (table);
    }

    position.setTargetValue (p.position);
    level.setTargetValue (p.level);

    const int newUnison = juce::jlimit (1, maxUnison, p.unisonCount);
    const bool tuningChanged = newUnison != numUnison
                            || p.coarse != coarse
                            || ! juce::approximatelyEqual (p.detuneCents, detune)
                            || ! juce::approximatelyEqual (p.spread, spread);

    numUnison = newUnison;
    coarse = p.coarse;
    detune = p.detuneCents;
    spread = p.spread;

    if (tuningChanged && noteHz > 0.0f)
        updateTuning (noteHz, sampleRate);
}

void SynthVoice::OscSection::updateTuning (float noteHz, double sampleRate)
{
    if (sampleRate <= 0.0)
        return;

    const float base = noteHz * std::exp2 ((float) coarse / 12.0f);

    for (int u = 0; u < numUnison; ++u)
    {
        const float offset = numUnison == 1
                               ? 0.0f
                               : -1.0f + 2.0f * (float) u / (float) (numUnison - 1);

        oscs[u].setFrequency (base * std::exp2 (detune * offset / 1200.0f), sampleRate);

        // equal-power pan, unison voices fanned out by the spread amount
        const float angle = (offset * spread + 1.0f) * juce::MathConstants<float>::pi / 4.0f;
        gainL[u] = std::cos (angle);
        gainR[u] = std::sin (angle);
    }

    unisonNorm = 1.0f / std::sqrt ((float) numUnison);
}

void SynthVoice::OscSection::noteStart (float noteHz, double sampleRate, juce::Random& rng)
{
    // random start phases keep stacked unison voices from comb-filtering
    for (auto& osc : oscs)
        osc.setPhase (rng.nextFloat());

    updateTuning (noteHz, sampleRate);
    position.setCurrentAndTargetValue (position.getTargetValue());
    level.setCurrentAndTargetValue (level.getTargetValue());
}

//==============================================================================
// SynthVoice

bool SynthVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*> (sound) != nullptr;
}

void SynthVoice::prepare (double sampleRate, int samplesPerBlock, int numOutputChannels)
{
    juce::dsp::ProcessSpec spec { sampleRate,
                                  (juce::uint32) samplesPerBlock,
                                  (juce::uint32) juce::jmax (1, numOutputChannels) };

    filter.prepare (spec);
    filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

    adsr.setSampleRate (sampleRate);

    oscA.reset (sampleRate);
    oscB.reset (sampleRate);
    subLevelSmoothed.reset (sampleRate, 0.02);
    noiseLevelSmoothed.reset (sampleRate, 0.02);

    voiceBuffer.setSize ((int) spec.numChannels, samplesPerBlock);

    isPrepared = true;
}

void SynthVoice::setParameters (const OscParams& oscAParams, const OscParams& oscBParams,
                                int subOctavesDown, float subLevel, float noiseLevel,
                                float attack, float decay, float sustain, float release,
                                float cutoffHz, float resonance)
{
    const auto sampleRate = getSampleRate();

    oscA.applyParams (oscAParams, noteHz, sampleRate);
    oscB.applyParams (oscBParams, noteHz, sampleRate);

    subLevelSmoothed.setTargetValue (subLevel);
    noiseLevelSmoothed.setTargetValue (noiseLevel);

    if (subOctavesDown != subOctave)
    {
        subOctave = subOctavesDown;
        updateSubInc();
    }

    adsrParams.attack = attack;
    adsrParams.decay = decay;
    adsrParams.sustain = sustain;
    adsrParams.release = release;
    adsr.setParameters (adsrParams);

    filter.setCutoffFrequency (cutoffHz);
    filter.setResonance (resonance);
}

void SynthVoice::updateSubInc()
{
    const auto sampleRate = getSampleRate();

    if (sampleRate > 0.0 && noteHz > 0.0f)
        subInc = (float) (noteHz * std::exp2 ((float) -subOctave) / sampleRate);
}

void SynthVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    noteHz = (float) juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

    const auto sampleRate = getSampleRate();
    oscA.noteStart (noteHz, sampleRate, random);
    oscB.noteStart (noteHz, sampleRate, random);

    subPhase = 0.0f;
    updateSubInc();
    subLevelSmoothed.setCurrentAndTargetValue (subLevelSmoothed.getTargetValue());
    noiseLevelSmoothed.setCurrentAndTargetValue (noiseLevelSmoothed.getTargetValue());

    filter.reset();

    level = 0.5f * velocity;
    adsr.noteOn();
}

void SynthVoice::stopNote (float, bool allowTailOff)
{
    adsr.noteOff();

    if (! allowTailOff || ! adsr.isActive())
        clearCurrentNote();
}

void SynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                  int startSample, int numSamples)
{
    jassert (isPrepared);

    if (! isVoiceActive() || numSamples == 0)
        return;

    voiceBuffer.setSize (outputBuffer.getNumChannels(), numSamples, false, false, true);
    voiceBuffer.clear();

    auto* left = voiceBuffer.getWritePointer (0);
    auto* right = voiceBuffer.getNumChannels() > 1 ? voiceBuffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        float l = 0.0f, r = 0.0f;

        oscA.renderSample (l, r);
        oscB.renderSample (l, r);

        const float subLev = subLevelSmoothed.getNextValue();

        if (subLev > 0.0001f)
        {
            const float s = std::sin (juce::MathConstants<float>::twoPi * subPhase) * subLev;
            l += s;
            r += s;
        }

        subPhase += subInc;
        if (subPhase >= 1.0f)
            subPhase -= 1.0f;

        const float noiseLev = noiseLevelSmoothed.getNextValue();

        if (noiseLev > 0.0001f)
        {
            l += (random.nextFloat() * 2.0f - 1.0f) * noiseLev;
            r += (random.nextFloat() * 2.0f - 1.0f) * noiseLev;
        }

        if (right != nullptr)
        {
            left[i] = l;
            right[i] = r;
        }
        else
        {
            left[i] = 0.5f * (l + r);
        }
    }

    juce::dsp::AudioBlock<float> block (voiceBuffer);
    filter.process (juce::dsp::ProcessContextReplacing<float> (block));
    adsr.applyEnvelopeToBuffer (voiceBuffer, 0, numSamples);

    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
        outputBuffer.addFrom (ch, startSample, voiceBuffer, ch, 0, numSamples, level);

    if (! adsr.isActive())
        clearCurrentNote();
}
