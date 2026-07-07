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

    void setupRotary (juce::Slider&, juce::Label&, const juce::String& text);

    AstralVegaAudioProcessor& processorRef;

    juce::ComboBox tableBox;
    juce::Label tableLabel;

    juce::Slider posSlider, unisonSlider, detuneSlider, spreadSlider;
    juce::Label posLabel, unisonLabel, detuneLabel, spreadLabel;

    juce::Slider cutoffSlider, resonanceSlider, gainSlider;
    juce::Label cutoffLabel, resonanceLabel, gainLabel;

    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel;

    std::unique_ptr<ComboBoxAttachment> tableAttachment;
    std::unique_ptr<SliderAttachment> posAttachment, unisonAttachment,
                                      detuneAttachment, spreadAttachment,
                                      cutoffAttachment, resonanceAttachment, gainAttachment,
                                      attackAttachment, decayAttachment,
                                      sustainAttachment, releaseAttachment;

    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AstralVegaAudioProcessorEditor)
};
