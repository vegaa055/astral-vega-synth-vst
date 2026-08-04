#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/** Shows one oscillator's wavetable: the frame currently selected by WT Pos
    drawn over ghosted frames from across the whole table, with a position
    cursor beneath. Polls its two parameters and only repaints on change. */
class WavetableDisplay : public juce::Component,
                         private juce::Timer
{
public:
    WavetableDisplay (AstralVegaAudioProcessor&, const juce::String& oscIdPrefix);

    void paint (juce::Graphics&) override;

private:
    using DisplayTable = AstralVegaAudioProcessor::DisplayTable;

    static constexpr int numGhostFrames = 5;
    static constexpr int maxColumns = 400;

    void timerCallback() override;
    static void buildFramePath (juce::Path&, const DisplayTable&,
                                float framePos, juce::Rectangle<float> area);

    AstralVegaAudioProcessor& processor;
    juce::String tableParamId, posParamId;

    int lastTableIndex = -1;
    float lastPosition = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableDisplay)
};
