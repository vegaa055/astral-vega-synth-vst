#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/** Shows one oscillator's wavetable: the frame currently sounding drawn over
    ghosted frames from across the whole table, with a position cursor beneath.

    While a voice is playing, the waveform follows that voice's *modulated*
    position, so LFO or envelope sweeps animate here; the unmodulated knob
    position stays visible as a dim tick. With nothing playing it falls back to
    the knob position. */
class WavetableDisplay : public juce::Component,
                         private juce::Timer
{
public:
    WavetableDisplay (AstralVegaAudioProcessor&, const juce::String& oscIdPrefix, int oscIndex);

    void paint (juce::Graphics&) override;

private:
    using DisplayTable = AstralVegaAudioProcessor::DisplayTable;

    static constexpr int numGhostFrames = 5;
    static constexpr int maxColumns = 400;

    void timerCallback() override;
    void rebuildGhosts (const DisplayTable&, juce::Rectangle<float> area);
    static void buildFramePath (juce::Path&, const DisplayTable&,
                                float framePos, juce::Rectangle<float> area);

    AstralVegaAudioProcessor& processor;
    juce::String tableParamId, posParamId;
    int oscIndex = 0;

    // ghosts only change with the table, so they survive the animated repaints
    juce::Path ghostPaths[numGhostFrames];
    int ghostTableIndex = -1, ghostGeneration = -1, ghostNumFrames = 0;
    juce::Rectangle<float> ghostArea;

    int lastTableIndex = -1;
    float lastBasePosition = -1.0f, lastShownPosition = -1.0f;
    bool lastLive = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableDisplay)
};
