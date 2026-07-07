#include "PluginEditor.h"

namespace
{
    const juce::Colour background { 0xff14101f };
    const juce::Colour neonMagenta { 0xffff2d95 };
    const juce::Colour neonCyan { 0xff29e6ff };

    void layoutRow (juce::Rectangle<int> row, std::initializer_list<juce::Component*> comps)
    {
        row.removeFromTop (20); // room for attached labels
        const int cellW = row.getWidth() / (int) comps.size();

        for (auto* comp : comps)
        {
            auto cell = row.removeFromLeft (cellW);

            if (dynamic_cast<juce::ComboBox*> (comp) != nullptr)
                comp->setBounds (cell.withSizeKeepingCentre (juce::jmax (56, cellW - 32), 28));
            else
                comp->setBounds (cell.reduced (6));
        }
    }
}

AstralVegaAudioProcessorEditor::AstralVegaAudioProcessorEditor (AstralVegaAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setupOscRow (oscARow, "oscA", "Osc A");
    setupOscRow (oscBRow, "oscB", "Osc B");

    setupCombo (subOctBox, subOctLabel, "Sub Oct",
                juce::StringArray { "-1 Oct", "-2 Oct" });

    setupCombo (voiceModeBox, voiceModeLabel, "Voice",
                juce::StringArray { "Poly", "Mono", "Legato" });
    setupRotary (glideSlider, glideLabel, "Glide");
    setupRotary (bendRangeSlider, bendRangeLabel, "Bend Range");
    addAndMakeVisible (pumpOnButton);
    setupRotary (pumpAmountSlider, pumpAmountLabel, "Amount");
    setupRotary (pumpRateSlider, pumpRateLabel, "Rate");

    setupRotary (subLevelSlider, subLevelLabel, "Sub Level");
    setupRotary (noiseSlider, noiseLabel, "Noise");
    setupRotary (cutoffSlider, cutoffLabel, "Cutoff");
    setupRotary (resonanceSlider, resonanceLabel, "Resonance");
    setupRotary (morphSlider, morphLabel, "Morph");
    setupRotary (driveSlider, driveLabel, "Drive");
    setupRotary (keytrackSlider, keytrackLabel, "Keytrack");
    setupRotary (envAmtSlider, envAmtLabel, "Env Amt");
    setupRotary (gainSlider, gainLabel, "Gain");
    setupRotary (attackSlider, attackLabel, "Attack");
    setupRotary (decaySlider, decayLabel, "Decay");
    setupRotary (sustainSlider, sustainLabel, "Sustain");
    setupRotary (releaseSlider, releaseLabel, "Release");

    const juce::StringArray lfoShapes { "Sine", "Triangle", "Saw Down", "Square", "S&H" };
    setupCombo (lfo1ShapeBox, lfo1ShapeLabel, "LFO 1", lfoShapes);
    setupCombo (lfo2ShapeBox, lfo2ShapeLabel, "LFO 2", lfoShapes);
    setupRotary (lfo1RateSlider, lfo1RateLabel, "Rate 1");
    setupRotary (lfo2RateSlider, lfo2RateLabel, "Rate 2");

    setupRotary (env2AttackSlider, env2AttackLabel, "Env2 A");
    setupRotary (env2DecaySlider, env2DecayLabel, "Env2 D");
    setupRotary (env2SustainSlider, env2SustainLabel, "Env2 S");
    setupRotary (env2ReleaseSlider, env2ReleaseLabel, "Env2 R");

    for (int s = 0; s < SynthVoice::numModSlots; ++s)
        setupModSlot (modSlots[s], s + 1);

    for (auto* button : { &distOnButton, &crushOnButton, &phaserOnButton,
                          &chorusOnButton, &delayOnButton, &reverbOnButton })
        addAndMakeVisible (*button);

    setupRotary (distDriveSlider, distDriveLabel, "Drive");
    setupRotary (distMixSlider, distMixLabel, "Mix");
    setupRotary (crushBitsSlider, crushBitsLabel, "Bits");
    setupRotary (crushRateSlider, crushRateLabel, "Downsample");
    setupRotary (phaserRateSlider, phaserRateLabel, "Rate");
    setupRotary (phaserDepthSlider, phaserDepthLabel, "Depth");
    setupRotary (phaserMixSlider, phaserMixLabel, "Mix");
    setupRotary (chorusRateSlider, chorusRateLabel, "Rate");
    setupRotary (chorusDepthSlider, chorusDepthLabel, "Depth");
    setupRotary (chorusMixSlider, chorusMixLabel, "Mix");
    setupRotary (delayTimeSlider, delayTimeLabel, "Time");
    setupRotary (delayFBSlider, delayFBLabel, "Feedback");
    setupRotary (delayMixSlider, delayMixLabel, "Mix");
    setupRotary (reverbSizeSlider, reverbSizeLabel, "Size");
    setupRotary (reverbDampSlider, reverbDampLabel, "Damp");
    setupRotary (reverbMixSlider, reverbMixLabel, "Mix");

    auto& apvts = processorRef.apvts;
    voiceModeAttachment = std::make_unique<ComboBoxAttachment> (apvts, "voiceMode", voiceModeBox);
    glideAttachment     = std::make_unique<SliderAttachment> (apvts, "glideTime", glideSlider);
    bendRangeAttachment = std::make_unique<SliderAttachment> (apvts, "pitchBendRange", bendRangeSlider);
    pumpOnAtt           = std::make_unique<ButtonAttachment> (apvts, "pumpOn", pumpOnButton);
    pumpAmountAtt       = std::make_unique<SliderAttachment> (apvts, "pumpAmount", pumpAmountSlider);
    pumpRateAtt         = std::make_unique<SliderAttachment> (apvts, "pumpRate", pumpRateSlider);

    distOnAtt    = std::make_unique<ButtonAttachment> (apvts, "distOn", distOnButton);
    crushOnAtt   = std::make_unique<ButtonAttachment> (apvts, "crushOn", crushOnButton);
    phaserOnAtt  = std::make_unique<ButtonAttachment> (apvts, "phaserOn", phaserOnButton);
    chorusOnAtt  = std::make_unique<ButtonAttachment> (apvts, "chorusOn", chorusOnButton);
    delayOnAtt   = std::make_unique<ButtonAttachment> (apvts, "delayOn", delayOnButton);
    reverbOnAtt  = std::make_unique<ButtonAttachment> (apvts, "reverbOn", reverbOnButton);

    distDriveAtt   = std::make_unique<SliderAttachment> (apvts, "distDrive", distDriveSlider);
    distMixAtt     = std::make_unique<SliderAttachment> (apvts, "distMix", distMixSlider);
    crushBitsAtt   = std::make_unique<SliderAttachment> (apvts, "crushBits", crushBitsSlider);
    crushRateAtt   = std::make_unique<SliderAttachment> (apvts, "crushRate", crushRateSlider);
    phaserRateAtt  = std::make_unique<SliderAttachment> (apvts, "phaserRate", phaserRateSlider);
    phaserDepthAtt = std::make_unique<SliderAttachment> (apvts, "phaserDepth", phaserDepthSlider);
    phaserMixAtt   = std::make_unique<SliderAttachment> (apvts, "phaserMix", phaserMixSlider);
    chorusRateAtt  = std::make_unique<SliderAttachment> (apvts, "chorusRate", chorusRateSlider);
    chorusDepthAtt = std::make_unique<SliderAttachment> (apvts, "chorusDepth", chorusDepthSlider);
    chorusMixAtt   = std::make_unique<SliderAttachment> (apvts, "chorusMix", chorusMixSlider);
    delayTimeAtt   = std::make_unique<SliderAttachment> (apvts, "delayTime", delayTimeSlider);
    delayFBAtt     = std::make_unique<SliderAttachment> (apvts, "delayFeedback", delayFBSlider);
    delayMixAtt    = std::make_unique<SliderAttachment> (apvts, "delayMix", delayMixSlider);
    reverbSizeAtt  = std::make_unique<SliderAttachment> (apvts, "reverbSize", reverbSizeSlider);
    reverbDampAtt  = std::make_unique<SliderAttachment> (apvts, "reverbDamp", reverbDampSlider);
    reverbMixAtt   = std::make_unique<SliderAttachment> (apvts, "reverbMix", reverbMixSlider);
    lfo1ShapeAttachment = std::make_unique<ComboBoxAttachment> (apvts, "lfo1Shape", lfo1ShapeBox);
    lfo2ShapeAttachment = std::make_unique<ComboBoxAttachment> (apvts, "lfo2Shape", lfo2ShapeBox);
    lfo1RateAttachment  = std::make_unique<SliderAttachment> (apvts, "lfo1Rate", lfo1RateSlider);
    lfo2RateAttachment  = std::make_unique<SliderAttachment> (apvts, "lfo2Rate", lfo2RateSlider);
    env2AttackAttachment  = std::make_unique<SliderAttachment> (apvts, "env2Attack", env2AttackSlider);
    env2DecayAttachment   = std::make_unique<SliderAttachment> (apvts, "env2Decay", env2DecaySlider);
    env2SustainAttachment = std::make_unique<SliderAttachment> (apvts, "env2Sustain", env2SustainSlider);
    env2ReleaseAttachment = std::make_unique<SliderAttachment> (apvts, "env2Release", env2ReleaseSlider);
    subOctAttachment    = std::make_unique<ComboBoxAttachment> (apvts, "subOctave", subOctBox);
    subLevelAttachment  = std::make_unique<SliderAttachment> (apvts, "subLevel", subLevelSlider);
    noiseAttachment     = std::make_unique<SliderAttachment> (apvts, "noiseLevel", noiseSlider);
    cutoffAttachment    = std::make_unique<SliderAttachment> (apvts, "filterCutoff", cutoffSlider);
    resonanceAttachment = std::make_unique<SliderAttachment> (apvts, "filterRes", resonanceSlider);
    morphAttachment     = std::make_unique<SliderAttachment> (apvts, "filterMorph", morphSlider);
    driveAttachment     = std::make_unique<SliderAttachment> (apvts, "filterDrive", driveSlider);
    keytrackAttachment  = std::make_unique<SliderAttachment> (apvts, "filterKeytrack", keytrackSlider);
    envAmtAttachment    = std::make_unique<SliderAttachment> (apvts, "filterEnvAmt", envAmtSlider);
    gainAttachment      = std::make_unique<SliderAttachment> (apvts, "gain", gainSlider);
    attackAttachment    = std::make_unique<SliderAttachment> (apvts, "attack", attackSlider);
    decayAttachment     = std::make_unique<SliderAttachment> (apvts, "decay", decaySlider);
    sustainAttachment   = std::make_unique<SliderAttachment> (apvts, "sustain", sustainSlider);
    releaseAttachment   = std::make_unique<SliderAttachment> (apvts, "release", releaseSlider);

    addAndMakeVisible (keyboard);

    setSize (1460, 760);
}

void AstralVegaAudioProcessorEditor::setupCombo (juce::ComboBox& box, juce::Label& label,
                                                 const juce::String& text,
                                                 const juce::StringArray& items)
{
    box.addItemList (items, 1);
    addAndMakeVisible (box);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.attachToComponent (&box, false);
}

void AstralVegaAudioProcessorEditor::setupModSlot (ModSlotControls& slot, int slotNumber)
{
    const juce::StringArray sources { "None", "LFO 1", "LFO 2", "Env 2", "Velocity", "Mod Wheel" };
    const juce::StringArray targets { "None", "A Pos", "B Pos", "A Level", "B Level",
                                      "Pitch", "Cutoff", "Reso", "Morph" };

    const auto num = juce::String (slotNumber);
    setupCombo (slot.src, slot.srcLabel, "Mod " + num, sources);
    setupCombo (slot.dst, slot.dstLabel, "Target", targets);
    setupRotary (slot.amt, slot.amtLabel, "Amount");

    const auto prefix = "mod" + num;
    auto& apvts = processorRef.apvts;
    slot.srcAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "Src", slot.src);
    slot.dstAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "Dst", slot.dst);
    slot.amtAtt = std::make_unique<SliderAttachment> (apvts, prefix + "Amt", slot.amt);
}

void AstralVegaAudioProcessorEditor::setupRotary (juce::Slider& slider, juce::Label& label,
                                                  const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 16);
    slider.setColour (juce::Slider::rotarySliderFillColourId, neonMagenta);
    slider.setColour (juce::Slider::thumbColourId, neonCyan);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.attachToComponent (&slider, false);
}

void AstralVegaAudioProcessorEditor::setupOscRow (OscRowControls& row,
                                                  const juce::String& idPrefix,
                                                  const juce::String& name)
{
    row.table.addItemList (juce::StringArray { "Basic", "PWM", "Spectra" }, 1);
    addAndMakeVisible (row.table);
    row.tableLabel.setText (name, juce::dontSendNotification);
    row.tableLabel.setJustificationType (juce::Justification::centred);
    row.tableLabel.attachToComponent (&row.table, false);

    setupRotary (row.pos, row.posLabel, "WT Pos");
    setupRotary (row.coarse, row.coarseLabel, "Coarse");
    setupRotary (row.unison, row.unisonLabel, "Unison");
    setupRotary (row.detune, row.detuneLabel, "Detune");
    setupRotary (row.spread, row.spreadLabel, "Spread");
    setupRotary (row.level, row.levelLabel, "Level");

    auto& apvts = processorRef.apvts;
    row.tableAtt  = std::make_unique<ComboBoxAttachment> (apvts, idPrefix + "Table", row.table);
    row.posAtt    = std::make_unique<SliderAttachment> (apvts, idPrefix + "Pos", row.pos);
    row.coarseAtt = std::make_unique<SliderAttachment> (apvts, idPrefix + "Coarse", row.coarse);
    row.unisonAtt = std::make_unique<SliderAttachment> (apvts, idPrefix + "Unison", row.unison);
    row.detuneAtt = std::make_unique<SliderAttachment> (apvts, idPrefix + "Detune", row.detune);
    row.spreadAtt = std::make_unique<SliderAttachment> (apvts, idPrefix + "Spread", row.spread);
    row.levelAtt  = std::make_unique<SliderAttachment> (apvts, idPrefix + "Level", row.level);
}

void AstralVegaAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);

    auto titleArea = getLocalBounds().removeFromTop (48).toFloat();
    g.setColour (neonMagenta);
    g.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
    g.drawText ("ASTRAL VEGA", titleArea, juce::Justification::centred);

    g.setColour (neonCyan.withAlpha (0.6f));
    g.drawLine (16.0f, titleArea.getBottom(),
                (float) getWidth() - 16.0f, titleArea.getBottom(), 1.5f);
}

void AstralVegaAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (16);

    keyboard.setBounds (bounds.removeFromBottom (90));
    bounds.removeFromTop (48); // title strip painted in paint()
    bounds.removeFromBottom (8);

    // left column: the synth engine; right column: modulation + FX rack
    auto left = bounds.removeFromLeft (juce::roundToInt ((float) bounds.getWidth() * 0.56f));
    auto right = bounds.withTrimmedLeft (12);

    const int leftRowH = left.getHeight() / 5;

    layoutRow (left.removeFromTop (leftRowH),
               { &oscARow.table, &oscARow.pos, &oscARow.coarse, &oscARow.unison,
                 &oscARow.detune, &oscARow.spread, &oscARow.level });

    layoutRow (left.removeFromTop (leftRowH),
               { &oscBRow.table, &oscBRow.pos, &oscBRow.coarse, &oscBRow.unison,
                 &oscBRow.detune, &oscBRow.spread, &oscBRow.level });

    layoutRow (left.removeFromTop (leftRowH),
               { &cutoffSlider, &resonanceSlider, &morphSlider,
                 &driveSlider, &keytrackSlider, &envAmtSlider });

    layoutRow (left.removeFromTop (leftRowH),
               { &subOctBox, &subLevelSlider, &noiseSlider, &attackSlider,
                 &decaySlider, &sustainSlider, &releaseSlider, &gainSlider });

    layoutRow (left,
               { &voiceModeBox, &glideSlider, &bendRangeSlider,
                 &pumpOnButton, &pumpAmountSlider, &pumpRateSlider });

    const int rightRowH = right.getHeight() / 6;

    layoutRow (right.removeFromTop (rightRowH),
               { &lfo1ShapeBox, &lfo1RateSlider, &lfo2ShapeBox, &lfo2RateSlider,
                 &env2AttackSlider, &env2DecaySlider, &env2SustainSlider, &env2ReleaseSlider });

    layoutRow (right.removeFromTop (rightRowH),
               { &modSlots[0].src, &modSlots[0].dst, &modSlots[0].amt,
                 &modSlots[1].src, &modSlots[1].dst, &modSlots[1].amt,
                 &modSlots[2].src, &modSlots[2].dst, &modSlots[2].amt });

    layoutRow (right.removeFromTop (rightRowH),
               { &modSlots[3].src, &modSlots[3].dst, &modSlots[3].amt,
                 &modSlots[4].src, &modSlots[4].dst, &modSlots[4].amt,
                 &modSlots[5].src, &modSlots[5].dst, &modSlots[5].amt });

    layoutRow (right.removeFromTop (rightRowH),
               { &distOnButton, &distDriveSlider, &distMixSlider,
                 &crushOnButton, &crushBitsSlider, &crushRateSlider });

    layoutRow (right.removeFromTop (rightRowH),
               { &phaserOnButton, &phaserRateSlider, &phaserDepthSlider, &phaserMixSlider,
                 &chorusOnButton, &chorusRateSlider, &chorusDepthSlider, &chorusMixSlider });

    layoutRow (right,
               { &delayOnButton, &delayTimeSlider, &delayFBSlider, &delayMixSlider,
                 &reverbOnButton, &reverbSizeSlider, &reverbDampSlider, &reverbMixSlider });
}
