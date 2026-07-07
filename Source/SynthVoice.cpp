#include "SynthVoice.h"

//==============================================================================
// OscSection

void SynthVoice::OscSection::reset (double sampleRate)
{
    position.reset (sampleRate, 0.01);
    level.reset (sampleRate, 0.01);
}

void SynthVoice::OscSection::applyParams (const OscParams& p, float effectiveHz, double sampleRate)
{
    if (p.table != table)
    {
        table = p.table;
        for (auto& osc : oscs)
            osc.setTable (table);
    }

    const int newUnison = juce::jlimit (1, maxUnison, p.unisonCount);
    const bool tuningChanged = newUnison != numUnison
                            || p.coarse != coarse
                            || ! juce::approximatelyEqual (p.detuneCents, detune)
                            || ! juce::approximatelyEqual (p.spread, spread);

    numUnison = newUnison;
    coarse = p.coarse;
    detune = p.detuneCents;
    spread = p.spread;

    if (tuningChanged && effectiveHz > 0.0f)
        updateTuning (effectiveHz, sampleRate);
}

void SynthVoice::OscSection::updateFrequencies (float effectiveHz, double sampleRate)
{
    if (sampleRate <= 0.0)
        return;

    const float base = effectiveHz * std::exp2 ((float) coarse / 12.0f);

    for (int u = 0; u < numUnison; ++u)
    {
        const float offset = numUnison == 1
                               ? 0.0f
                               : -1.0f + 2.0f * (float) u / (float) (numUnison - 1);

        oscs[u].setFrequency (base * std::exp2 (detune * offset / 1200.0f), sampleRate);
    }
}

void SynthVoice::OscSection::updatePanGains()
{
    for (int u = 0; u < numUnison; ++u)
    {
        const float offset = numUnison == 1
                               ? 0.0f
                               : -1.0f + 2.0f * (float) u / (float) (numUnison - 1);

        // equal-power pan, unison voices fanned out by the spread amount
        const float angle = (offset * spread + 1.0f) * juce::MathConstants<float>::pi / 4.0f;
        gainL[u] = std::cos (angle);
        gainR[u] = std::sin (angle);
    }

    unisonNorm = 1.0f / std::sqrt ((float) numUnison);
}

void SynthVoice::OscSection::updateTuning (float effectiveHz, double sampleRate)
{
    updateFrequencies (effectiveHz, sampleRate);
    updatePanGains();
}

void SynthVoice::OscSection::noteStart (const OscParams& p, float effectiveHz,
                                        double sampleRate, juce::Random& rng)
{
    // random start phases keep stacked unison voices from comb-filtering
    for (auto& osc : oscs)
        osc.setPhase (rng.nextFloat());

    updateTuning (effectiveHz, sampleRate);
    position.setCurrentAndTargetValue (p.position);
    level.setCurrentAndTargetValue (p.level);
}

//==============================================================================
// SynthVoice

bool SynthVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*> (sound) != nullptr;
}

void SynthVoice::prepare (double sampleRate, int samplesPerBlock, int numOutputChannels)
{
    const int channels = juce::jmax (1, numOutputChannels);

    filter.prepare (sampleRate, channels);

    adsr.setSampleRate (sampleRate);
    env2.setSampleRate (sampleRate);
    lfo1.prepare (sampleRate);
    lfo2.prepare (sampleRate);

    oscA.reset (sampleRate);
    oscB.reset (sampleRate);
    subLevelSmoothed.reset (sampleRate, 0.02);
    noiseLevelSmoothed.reset (sampleRate, 0.02);

    voiceBuffer.setSize (channels, samplesPerBlock);

    isPrepared = true;
}

void SynthVoice::setParameters (const BlockParams& p)
{
    params = p;

    const auto sampleRate = getSampleRate();
    const float effectiveHz = noteHz * pitchMul;

    oscA.applyParams (p.oscA, effectiveHz, sampleRate);
    oscB.applyParams (p.oscB, effectiveHz, sampleRate);

    subLevelSmoothed.setTargetValue (p.subLevel);
    noiseLevelSmoothed.setTargetValue (p.noiseLevel);

    if (p.subOctavesDown != subOctave)
    {
        subOctave = p.subOctavesDown;
        updateSubInc();
    }

    lfo1.setParams (p.lfo1Shape, p.lfo1Rate);
    lfo2.setParams (p.lfo2Shape, p.lfo2Rate);

    adsrParams.attack = p.attack;
    adsrParams.decay = p.decay;
    adsrParams.sustain = p.sustain;
    adsrParams.release = p.release;
    adsr.setParameters (adsrParams);

    env2Params.attack = p.env2Attack;
    env2Params.decay = p.env2Decay;
    env2Params.sustain = p.env2Sustain;
    env2Params.release = p.env2Release;
    env2.setParameters (env2Params);

    // cutoff/resonance and modulated targets are applied per control chunk
    // in updateModulation()
}

void SynthVoice::updateSubInc()
{
    const auto sampleRate = getSampleRate();

    if (sampleRate > 0.0 && noteHz > 0.0f)
        subInc = (float) (noteHz * pitchMul * std::exp2 ((float) -subOctave) / sampleRate);
}

void SynthVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    noteHz = (float) juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    noteNumber = midiNoteNumber;
    pitchMul = 1.0f;
    currentPitchSemis = 0.0f;
    velocity01 = velocity;

    const auto sampleRate = getSampleRate();
    oscA.noteStart (params.oscA, noteHz, sampleRate, random);
    oscB.noteStart (params.oscB, noteHz, sampleRate, random);

    subPhase = 0.0f;
    updateSubInc();
    subLevelSmoothed.setCurrentAndTargetValue (subLevelSmoothed.getTargetValue());
    noiseLevelSmoothed.setCurrentAndTargetValue (noiseLevelSmoothed.getTargetValue());

    lfo1.noteOn (random);
    lfo2.noteOn (random);

    filter.reset();

    level = 0.5f * velocity;
    adsr.noteOn();
    env2.noteOn();
}

void SynthVoice::stopNote (float, bool allowTailOff)
{
    adsr.noteOff();
    env2.noteOff();

    if (! allowTailOff || ! adsr.isActive())
        clearCurrentNote();
}

void SynthVoice::updateModulation (int chunkSamples)
{
    float src[numModSources];
    src[srcNone] = 0.0f;
    src[srcLFO1] = lfo1.tick (chunkSamples, random);
    src[srcLFO2] = lfo2.tick (chunkSamples, random);

    float env2Value = 0.0f;
    for (int k = 0; k < chunkSamples; ++k)
        env2Value = env2.getNextSample();
    src[srcEnv2] = env2Value;

    src[srcVelocity] = velocity01;
    src[srcModWheel] = params.modWheel;

    float mod[numModTargets] {};

    for (const auto& routing : params.routings)
        if (routing.source != srcNone && routing.target != tgtNone)
            mod[routing.target] += src[routing.source] * routing.amount;

    oscA.position.setTargetValue (juce::jlimit (0.0f, 1.0f, params.oscA.position + mod[tgtOscAPos]));
    oscB.position.setTargetValue (juce::jlimit (0.0f, 1.0f, params.oscB.position + mod[tgtOscBPos]));
    oscA.level.setTargetValue (juce::jlimit (0.0f, 1.0f, params.oscA.level + mod[tgtOscALevel]));
    oscB.level.setTargetValue (juce::jlimit (0.0f, 1.0f, params.oscB.level + mod[tgtOscBLevel]));

    // full amount = +/- one octave
    const float pitchSemis = mod[tgtPitch] * 12.0f;

    if (! juce::approximatelyEqual (pitchSemis, currentPitchSemis))
    {
        currentPitchSemis = pitchSemis;
        pitchMul = std::exp2 (pitchSemis / 12.0f);

        const auto sampleRate = getSampleRate();
        oscA.updateFrequencies (noteHz * pitchMul, sampleRate);
        oscB.updateFrequencies (noteHz * pitchMul, sampleRate);
        updateSubInc();
    }

    // cutoff: key tracking + dedicated env amount + matrix, all in octaves
    // (full matrix/env amount = +/- six octaves of sweep)
    const float keytrackOct = params.filterKeytrack * (float) (noteNumber - 60) / 12.0f;
    const float envOct = src[srcEnv2] * params.filterEnvAmt;

    const float cutoff = juce::jlimit (20.0f, 20000.0f,
        params.cutoffHz * std::exp2 (keytrackOct + (mod[tgtCutoff] + envOct) * 6.0f));

    const float resonance = juce::jlimit (0.5f, 8.0f,
        params.resonance + mod[tgtResonance] * 7.5f);

    const float morph = juce::jlimit (0.0f, 1.0f,
        params.filterMorph + mod[tgtFilterMorph]);

    filter.setParams (cutoff, resonance, morph, params.filterDrive);
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

    int done = 0;

    while (done < numSamples)
    {
        const int chunk = juce::jmin (controlInterval, numSamples - done);

        updateModulation (chunk);

        for (int i = done; i < done + chunk; ++i)
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

        filter.processBlock (voiceBuffer, done, chunk);

        done += chunk;
    }

    adsr.applyEnvelopeToBuffer (voiceBuffer, 0, numSamples);

    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
        outputBuffer.addFrom (ch, startSample, voiceBuffer, ch, 0, numSamples, level);

    if (! adsr.isActive())
        clearCurrentNote();
}
