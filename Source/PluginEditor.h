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

    struct ModSlotControls
    {
        juce::ComboBox src, dst;
        juce::Slider amt;
        juce::Label srcLabel, dstLabel, amtLabel;

        std::unique_ptr<ComboBoxAttachment> srcAtt, dstAtt;
        std::unique_ptr<SliderAttachment> amtAtt;
    };

    void setupRotary (juce::Slider&, juce::Label&, const juce::String& text);
    void setupCombo (juce::ComboBox&, juce::Label&, const juce::String& text,
                     const juce::StringArray& items);
    void setupOscRow (OscRowControls&, const juce::String& idPrefix, const juce::String& name);
    void setupModSlot (ModSlotControls&, int slotNumber);

    AstralVegaAudioProcessor& processorRef;

    OscRowControls oscARow, oscBRow;

    juce::ComboBox lfo1ShapeBox, lfo2ShapeBox;
    juce::Label lfo1ShapeLabel, lfo2ShapeLabel;
    juce::Slider lfo1RateSlider, lfo2RateSlider;
    juce::Label lfo1RateLabel, lfo2RateLabel;

    juce::Slider env2AttackSlider, env2DecaySlider, env2SustainSlider, env2ReleaseSlider;
    juce::Label env2AttackLabel, env2DecayLabel, env2SustainLabel, env2ReleaseLabel;

    ModSlotControls modSlots[SynthVoice::numModSlots];

    std::unique_ptr<ComboBoxAttachment> lfo1ShapeAttachment, lfo2ShapeAttachment;
    std::unique_ptr<SliderAttachment> lfo1RateAttachment, lfo2RateAttachment,
                                      env2AttackAttachment, env2DecayAttachment,
                                      env2SustainAttachment, env2ReleaseAttachment;

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
