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
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

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

    void refreshPresetBox();

    AstralVegaAudioProcessor& processorRef;

    juce::TextButton prevPresetButton { "<" }, nextPresetButton { ">" },
                     savePresetButton { "Save" };
    juce::ComboBox presetBox;
    std::unique_ptr<juce::FileChooser> fileChooser;

    OscRowControls oscARow, oscBRow;

    juce::ComboBox lfo1ShapeBox, lfo2ShapeBox;
    juce::Label lfo1ShapeLabel, lfo2ShapeLabel;
    juce::Slider lfo1RateSlider, lfo2RateSlider;
    juce::Label lfo1RateLabel, lfo2RateLabel;

    juce::Slider env2AttackSlider, env2DecaySlider, env2SustainSlider, env2ReleaseSlider;
    juce::Label env2AttackLabel, env2DecayLabel, env2SustainLabel, env2ReleaseLabel;

    ModSlotControls modSlots[SynthVoice::numModSlots];

    juce::ToggleButton distOnButton { "Dist" }, crushOnButton { "Crush" },
                       phaserOnButton { "Phaser" }, chorusOnButton { "Chorus" },
                       delayOnButton { "Delay" }, reverbOnButton { "Reverb" };

    juce::Slider distDriveSlider, distMixSlider, crushBitsSlider, crushRateSlider,
                 phaserRateSlider, phaserDepthSlider, phaserMixSlider,
                 chorusRateSlider, chorusDepthSlider, chorusMixSlider,
                 delayTimeSlider, delayFBSlider, delayMixSlider,
                 reverbSizeSlider, reverbDampSlider, reverbMixSlider;

    juce::Label distDriveLabel, distMixLabel, crushBitsLabel, crushRateLabel,
                phaserRateLabel, phaserDepthLabel, phaserMixLabel,
                chorusRateLabel, chorusDepthLabel, chorusMixLabel,
                delayTimeLabel, delayFBLabel, delayMixLabel,
                reverbSizeLabel, reverbDampLabel, reverbMixLabel;

    std::unique_ptr<ButtonAttachment> distOnAtt, crushOnAtt, phaserOnAtt,
                                      chorusOnAtt, delayOnAtt, reverbOnAtt;
    std::unique_ptr<SliderAttachment> distDriveAtt, distMixAtt, crushBitsAtt, crushRateAtt,
                                      phaserRateAtt, phaserDepthAtt, phaserMixAtt,
                                      chorusRateAtt, chorusDepthAtt, chorusMixAtt,
                                      delayTimeAtt, delayFBAtt, delayMixAtt,
                                      reverbSizeAtt, reverbDampAtt, reverbMixAtt;

    std::unique_ptr<ComboBoxAttachment> lfo1ShapeAttachment, lfo2ShapeAttachment;
    std::unique_ptr<SliderAttachment> lfo1RateAttachment, lfo2RateAttachment,
                                      env2AttackAttachment, env2DecayAttachment,
                                      env2SustainAttachment, env2ReleaseAttachment;

    juce::ComboBox subOctBox;
    juce::Label subOctLabel;

    juce::ComboBox voiceModeBox;
    juce::Label voiceModeLabel;
    juce::Slider glideSlider, bendRangeSlider;
    juce::Label glideLabel, bendRangeLabel;
    juce::ToggleButton pumpOnButton { "Pump" };
    juce::Slider pumpAmountSlider, pumpRateSlider;
    juce::Label pumpAmountLabel, pumpRateLabel;

    juce::Slider subLevelSlider, noiseSlider, cutoffSlider, resonanceSlider, gainSlider;
    juce::Label subLevelLabel, noiseLabel, cutoffLabel, resonanceLabel, gainLabel;

    juce::Slider morphSlider, driveSlider, keytrackSlider, envAmtSlider;
    juce::Label morphLabel, driveLabel, keytrackLabel, envAmtLabel;

    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel;

    std::unique_ptr<ComboBoxAttachment> subOctAttachment, voiceModeAttachment;
    std::unique_ptr<ButtonAttachment> pumpOnAtt;
    std::unique_ptr<SliderAttachment> glideAttachment, bendRangeAttachment,
                                      pumpAmountAtt, pumpRateAtt,
                                      subLevelAttachment, noiseAttachment,
                                      cutoffAttachment, resonanceAttachment, gainAttachment,
                                      morphAttachment, driveAttachment,
                                      keytrackAttachment, envAmtAttachment,
                                      attackAttachment, decayAttachment,
                                      sustainAttachment, releaseAttachment;

    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AstralVegaAudioProcessorEditor)
};
