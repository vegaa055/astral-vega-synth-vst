#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "AstralLookAndFeel.h"
#include "ModDragDrop.h"
#include "Visualizer.h"
#include "WavetableDisplay.h"

class AstralVegaAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       public juce::DragAndDropContainer
{
public:
    explicit AstralVegaAudioProcessorEditor (AstralVegaAudioProcessor&);
    ~AstralVegaAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool keyStateChanged (bool isKeyDown) override;
    void focusLost (FocusChangeType) override;
    void focusOfChildComponentChanged (FocusChangeType) override;

private:
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct OscRowControls
    {
        juce::ComboBox table;
        juce::Label tableLabel;
        ModTargetSlider pos, level;
        juce::Slider coarse, unison, detune, spread;
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

    struct Panel
    {
        juce::Rectangle<int> area;
        juce::String title;
    };

    void setupRotary (juce::Slider&, juce::Label&, const juce::String& text);
    void setupCombo (juce::ComboBox&, juce::Label&, const juce::String& text,
                     const juce::StringArray& items);
    void setupOscRow (OscRowControls&, const juce::String& idPrefix);
    void setupModSlot (ModSlotControls&, int slotNumber);
    void refreshPresetBox();
    void assignModRouting (int sourceIndex, int targetIndex);

    /** Registers a panel for paint() and returns its content area. */
    juce::Rectangle<int> addPanel (juce::Rectangle<int> area, const juce::String& title);

    AstralVegaAudioProcessor& processorRef;

    AstralLookAndFeel lookAndFeel;   // declared before all components

    juce::TextButton prevPresetButton { "<" }, nextPresetButton { ">" },
                     savePresetButton { "Save" }, loadTableButton { "Load WT" };
    juce::ComboBox presetBox;
    std::unique_ptr<juce::FileChooser> fileChooser;

    Visualizer visualizer;
    WavetableDisplay wtDisplayA, wtDisplayB;

    OscRowControls oscARow, oscBRow;

    ModTargetSlider cutoffSlider, resonanceSlider, morphSlider;
    juce::Slider driveSlider, keytrackSlider, envAmtSlider;
    juce::Label cutoffLabel, resonanceLabel, morphLabel, driveLabel, keytrackLabel, envAmtLabel;

    juce::ComboBox subOctBox;
    juce::Label subOctLabel;
    juce::Slider subLevelSlider, noiseSlider, gainSlider;
    juce::Label subLevelLabel, noiseLabel, gainLabel;

    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel;

    juce::ComboBox voiceModeBox;
    juce::Label voiceModeLabel;
    juce::Slider glideSlider, bendRangeSlider;
    juce::Label glideLabel, bendRangeLabel;
    juce::ToggleButton pumpOnButton { "Pump" }, pumpSyncButton { "Sync" };
    juce::ComboBox pumpDivBox;
    juce::Label pumpDivLabel;
    juce::Slider pumpAmountSlider, pumpRateSlider;
    juce::Label pumpAmountLabel, pumpRateLabel;

    juce::ComboBox lfo1ShapeBox, lfo2ShapeBox;
    juce::Label lfo1ShapeLabel, lfo2ShapeLabel;
    juce::ToggleButton lfo1SyncButton { "Sync" }, lfo1FreeButton { "Free" },
                       lfo2SyncButton { "Sync" }, lfo2FreeButton { "Free" };
    juce::ComboBox lfo1DivBox, lfo2DivBox;
    juce::Label lfo1DivLabel, lfo2DivLabel;
    juce::Slider lfo1RateSlider, lfo2RateSlider;
    juce::Label lfo1RateLabel, lfo2RateLabel;

    juce::Slider env2AttackSlider, env2DecaySlider, env2SustainSlider, env2ReleaseSlider;
    juce::Label env2AttackLabel, env2DecayLabel, env2SustainLabel, env2ReleaseLabel;

    ModSourceChip chipLFO1 { SynthVoice::srcLFO1, "LFO 1" },
                  chipLFO2 { SynthVoice::srcLFO2, "LFO 2" },
                  chipEnv2 { SynthVoice::srcEnv2, "ENV 2" },
                  chipVel  { SynthVoice::srcVelocity, "VEL" },
                  chipWheel { SynthVoice::srcModWheel, "WHEEL" };

    ModSlotControls modSlots[SynthVoice::numModSlots];

    juce::ToggleButton distOnButton { "Dist" }, crushOnButton { "Crush" },
                       phaserOnButton { "Phaser" }, chorusOnButton { "Chorus" },
                       delayOnButton { "Delay" }, delaySyncButton { "Sync" },
                       reverbOnButton { "Reverb" };
    juce::ComboBox delayDivBox;
    juce::Label delayDivLabel;

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

    std::unique_ptr<ComboBoxAttachment> voiceModeAttachment, subOctAttachment,
                                        lfo1ShapeAttachment, lfo2ShapeAttachment,
                                        lfo1DivAtt, lfo2DivAtt, pumpDivAtt, delayDivAtt;

    std::unique_ptr<ButtonAttachment> pumpOnAtt, pumpSyncAtt,
                                      lfo1SyncAtt, lfo1FreeAtt, lfo2SyncAtt, lfo2FreeAtt,
                                      distOnAtt, crushOnAtt, phaserOnAtt, chorusOnAtt,
                                      delayOnAtt, delaySyncAtt, reverbOnAtt;

    std::unique_ptr<SliderAttachment> cutoffAttachment, resonanceAttachment, morphAttachment,
                                      driveAttachment, keytrackAttachment, envAmtAttachment,
                                      subLevelAttachment, noiseAttachment, gainAttachment,
                                      attackAttachment, decayAttachment,
                                      sustainAttachment, releaseAttachment,
                                      glideAttachment, bendRangeAttachment,
                                      pumpAmountAtt, pumpRateAtt,
                                      lfo1RateAttachment, lfo2RateAttachment,
                                      env2AttackAttachment, env2DecayAttachment,
                                      env2SustainAttachment, env2ReleaseAttachment,
                                      distDriveAtt, distMixAtt, crushBitsAtt, crushRateAtt,
                                      phaserRateAtt, phaserDepthAtt, phaserMixAtt,
                                      chorusRateAtt, chorusDepthAtt, chorusMixAtt,
                                      delayTimeAtt, delayFBAtt, delayMixAtt,
                                      reverbSizeAtt, reverbDampAtt, reverbMixAtt;

    /** One QWERTY key of the FL-style typing keyboard. */
    struct TypingKey
    {
        int keyCode;
        int offset;             // semitones above the base octave's C
        int playingNote = -1;   // note actually sounding, -1 when up
    };

    void initTypingKeys();
    void setKeyboardOctave (int newOctave);
    void releaseAllTypingNotes();

    std::vector<TypingKey> typingKeys;
    int keyboardOctave = 4;

    juce::TextButton octDownButton { "-" }, octUpButton { "+" };
    juce::Label octLabel;

    juce::MidiKeyboardComponent keyboard;

    std::vector<Panel> panels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AstralVegaAudioProcessorEditor)
};
