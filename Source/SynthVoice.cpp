#include "SynthVoice.h"

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
    positionSmoothed.reset (sampleRate, 0.05);

    voiceBuffer.setSize ((int) spec.numChannels, samplesPerBlock);

    isPrepared = true;
}

void SynthVoice::setParameters (const Wavetable* newTable, float position,
                                int unisonCount, float detuneCents, float spreadAmount,
                                float attack, float decay, float sustain, float release,
                                float cutoffHz, float resonance)
{
    if (newTable != table)
    {
        table = newTable;
        for (auto& osc : oscs)
            osc.setTable (table);
    }

    positionSmoothed.setTargetValue (position);

    unisonCount = juce::jlimit (1, maxUnison, unisonCount);

    if (unisonCount != numUnison
        || ! juce::approximatelyEqual (detuneCents, detune)
        || ! juce::approximatelyEqual (spreadAmount, spread))
    {
        numUnison = unisonCount;
        detune = detuneCents;
        spread = spreadAmount;

        if (noteHz > 0.0f)
            updateUnison();
    }

    adsrParams.attack = attack;
    adsrParams.decay = decay;
    adsrParams.sustain = sustain;
    adsrParams.release = release;
    adsr.setParameters (adsrParams);

    filter.setCutoffFrequency (cutoffHz);
    filter.setResonance (resonance);
}

void SynthVoice::updateUnison()
{
    const auto sampleRate = getSampleRate();

    if (sampleRate <= 0.0)
        return;

    for (int u = 0; u < numUnison; ++u)
    {
        const float offset = numUnison == 1
                               ? 0.0f
                               : -1.0f + 2.0f * (float) u / (float) (numUnison - 1);

        oscs[u].setFrequency (noteHz * std::exp2 (detune * offset / 1200.0f), sampleRate);

        // equal-power pan, unison voices fanned out by the spread amount
        const float angle = (offset * spread + 1.0f) * juce::MathConstants<float>::pi / 4.0f;
        gainL[u] = std::cos (angle);
        gainR[u] = std::sin (angle);
    }

    unisonNorm = 1.0f / std::sqrt ((float) numUnison);
}

void SynthVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    noteHz = (float) juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

    // random start phases keep stacked unison voices from comb-filtering
    for (auto& osc : oscs)
        osc.setPhase (random.nextFloat());

    updateUnison();
    filter.reset();
    positionSmoothed.setCurrentAndTargetValue (positionSmoothed.getTargetValue());

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

    if (! isVoiceActive() || numSamples == 0 || table == nullptr)
        return;

    voiceBuffer.setSize (outputBuffer.getNumChannels(), numSamples, false, false, true);
    voiceBuffer.clear();

    auto* left = voiceBuffer.getWritePointer (0);
    auto* right = voiceBuffer.getNumChannels() > 1 ? voiceBuffer.getWritePointer (1) : nullptr;

    const float frameScale = (float) (table->numFrames - 1);

    for (int i = 0; i < numSamples; ++i)
    {
        const float framePos = positionSmoothed.getNextValue() * frameScale;
        float l = 0.0f, r = 0.0f;

        for (int u = 0; u < numUnison; ++u)
        {
            const float s = oscs[u].getNextSample (framePos);
            l += s * gainL[u];
            r += s * gainR[u];
        }

        if (right != nullptr)
        {
            left[i] = l * unisonNorm;
            right[i] = r * unisonNorm;
        }
        else
        {
            left[i] = 0.5f * (l + r) * unisonNorm;
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
