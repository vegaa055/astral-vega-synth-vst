#include "FXChain.h"

void FXChain::prepare (double sampleRateIn, int samplesPerBlock, int numChannelsIn)
{
    sampleRate = sampleRateIn;
    numChannels = juce::jlimit (1, 2, numChannelsIn);

    juce::dsp::ProcessSpec spec { sampleRate,
                                  (juce::uint32) samplesPerBlock,
                                  (juce::uint32) numChannels };

    phaser.prepare (spec);
    phaser.setCentreFrequency (800.0f);
    phaser.setFeedback (0.5f);

    chorus.prepare (spec);
    chorus.setCentreDelay (7.0f);
    chorus.setFeedback (0.0f);

    delayLine.prepare (spec);
    delayLine.setMaximumDelayInSamples ((int) (sampleRate * 2.0) + 1);
    delaySamplesSmoothed.reset (sampleRate, 0.05);

    reverb.prepare (spec);

    crushHold[0] = crushHold[1] = 0.0f;
    crushCounter[0] = crushCounter[1] = 0;
}

void FXChain::setParams (const Params& p)
{
    // reset state on enable edges so stale tails don't play back
    if (p.phaserOn && ! params.phaserOn)
        phaser.reset();

    if (p.chorusOn && ! params.chorusOn)
        chorus.reset();

    if (p.delayOn && ! params.delayOn)
    {
        delayLine.reset();
        delaySamplesSmoothed.setCurrentAndTargetValue ((float) (p.delayMs * 0.001 * sampleRate));
    }

    if (p.reverbOn && ! params.reverbOn)
        reverb.reset();

    if (p.crushOn && ! params.crushOn)
        crushCounter[0] = crushCounter[1] = 0;

    if (p.pumpOn && ! params.pumpOn)
        pumpPhase = 0.0f;

    params = p;

    phaser.setRate (p.phaserRate);
    phaser.setDepth (p.phaserDepth);
    phaser.setMix (p.phaserMix);

    chorus.setRate (p.chorusRate);
    chorus.setDepth (p.chorusDepth);
    chorus.setMix (p.chorusMix);

    delaySamplesSmoothed.setTargetValue ((float) (p.delayMs * 0.001 * sampleRate));

    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize = p.reverbSize;
    reverbParams.damping = p.reverbDamp;
    reverbParams.wetLevel = p.reverbMix;
    reverbParams.dryLevel = 1.0f - p.reverbMix;
    reverbParams.width = 1.0f;
    reverb.setParameters (reverbParams);
}

void FXChain::process (juce::AudioBuffer<float>& buffer)
{
    if (params.distOn)
        processDistortion (buffer);

    if (params.crushOn)
        processBitcrusher (buffer);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    if (params.phaserOn)
        phaser.process (context);

    if (params.chorusOn)
        chorus.process (context);

    if (params.delayOn)
        processDelay (buffer);

    if (params.reverbOn)
        reverb.process (context);

    if (params.pumpOn)
        processPump (buffer);
}

void FXChain::processPump (juce::AudioBuffer<float>& buffer)
{
    // when synced, lock the duck to the host timeline so renders sit on the grid
    if (params.pumpSync && sampleRate > 0.0)
    {
        const double cycles = (double) params.blockStartSample * params.pumpRate / sampleRate;
        pumpPhase = (float) (cycles - std::floor (cycles));
    }

    // sidechain-style duck: hard dip at each cycle start, quadratic recovery
    auto* const* data = buffer.getArrayOfWritePointers();
    const int channels = buffer.getNumChannels();
    const float inc = (float) (params.pumpRate / sampleRate);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float shape = 1.0f - pumpPhase;
        const float gain = 1.0f - params.pumpAmount * shape * shape;

        for (int ch = 0; ch < channels; ++ch)
            data[ch][i] *= gain;

        pumpPhase += inc;
        if (pumpPhase >= 1.0f)
            pumpPhase -= 1.0f;
    }
}

void FXChain::processDistortion (juce::AudioBuffer<float>& buffer)
{
    const float pre = 1.0f + params.distDrive * 24.0f;
    const float makeup = 1.0f / std::sqrt (pre);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer (ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = data[i];
            const float wet = std::tanh (dry * pre) * makeup;
            data[i] = dry + (wet - dry) * params.distMix;
        }
    }
}

void FXChain::processBitcrusher (juce::AudioBuffer<float>& buffer)
{
    const float levels = std::exp2 ((float) params.crushBits - 1.0f);
    const int rate = juce::jmax (1, params.crushRate);
    const int channels = juce::jmin (buffer.getNumChannels(), 2);

    for (int ch = 0; ch < channels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        float hold = crushHold[ch];
        int counter = crushCounter[ch];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (counter == 0)
                hold = std::round (data[i] * levels) / levels;

            data[i] = hold;

            if (++counter >= rate)
                counter = 0;
        }

        crushHold[ch] = hold;
        crushCounter[ch] = counter;
    }
}

void FXChain::processDelay (juce::AudioBuffer<float>& buffer)
{
    const int channels = juce::jmin (buffer.getNumChannels(), numChannels);
    auto* const* data = buffer.getArrayOfWritePointers();

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float delaySamples = delaySamplesSmoothed.getNextValue();

        for (int ch = 0; ch < channels; ++ch)
        {
            const float in = data[ch][i];
            const float delayed = delayLine.popSample (ch, delaySamples, true);

            delayLine.pushSample (ch, in + delayed * params.delayFeedback);
            data[ch][i] = in + delayed * params.delayMix;
        }
    }
}
