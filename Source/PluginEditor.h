#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class AstralVegaAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit AstralVegaAudioProcessorEditor (AstralVegaAudioProcessor&);
    ~AstralVegaAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct OscRowControls
    {
        juce::ComboBox table;
        juce::Label tableLabel;
        juce::Slider pos, coarse, unison, detune, spread, level;
        juce::Label posLabel, coarseLabel, unisonLabel, detuneLabel, spreadLabel, levelLabel;

        std::unique_ptr<ComboBoxAttachment> tableAtt;
        std::unique_ptr<SliderAttachment> posAtt, coarseAtt, unisonAtt,
                                          detuneAtt, spreadAtt, levelAtt;
    };

    void setupRotary (juce::Slider&, juce::Label&, const juce::String& text);
    void setupOscRow (OscRowControls&, const juce::String& idPrefix, const juce::String& name);

    AstralVegaAudioProcessor& processorRef;

    OscRowControls oscARow, oscBRow;

    juce::ComboBox subOctBox;
    juce::Label subOctLabel;

    juce::Slider subLevelSlider, noiseSlider, cutoffSlider, resonanceSlider, gainSlider;
    juce::Label subLevelLabel, noiseLabel, cutoffLabel, resonanceLabel, gainLabel;

    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel;

    std::unique_ptr<ComboBoxAttachment> subOctAttachment;
    std::unique_ptr<SliderAttachment> subLevelAttachment, noiseAttachment,
                                      cutoffAttachment, resonanceAttachment, gainAttachment,
                                      attackAttachment, decayAttachment,
                                      sustainAttachment, releaseAttachment;

    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AstralVegaAudioProcessorEditor)
};
