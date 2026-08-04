#include "PluginEditor.h"
#include "TempoDivisions.h"

namespace
{
    void layoutRow (juce::Rectangle<int> row, std::initializer_list<juce::Component*> comps)
    {
        row.removeFromTop (14); // room for attached labels
        const int cellW = row.getWidth() / (int) comps.size();

        for (auto* comp : comps)
        {
            auto cell = row.removeFromLeft (cellW);

            if (dynamic_cast<juce::ComboBox*> (comp) != nullptr)
                comp->setBounds (cell.withSizeKeepingCentre (juce::jmax (56, cellW - 16), 26));
            else
                comp->setBounds (cell.reduced (4));
        }
    }
}

AstralVegaAudioProcessorEditor::AstralVegaAudioProcessorEditor (AstralVegaAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      visualizer (p),
      wtDisplayA (p, "oscA", 0), wtDisplayB (p, "oscB", 1),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lookAndFeel);

    // --- preset bar ------------------------------------------------------
    for (auto* c : { (juce::Component*) &prevPresetButton, (juce::Component*) &nextPresetButton,
                     (juce::Component*) &savePresetButton, (juce::Component*) &loadTableButton,
                     (juce::Component*) &presetBox })
        addAndMakeVisible (*c);

    refreshPresetBox();

    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedItemIndex();

        if (idx >= 0 && idx != processorRef.presetManager.getCurrentIndex())
            processorRef.presetManager.loadPreset (idx);
    };

    prevPresetButton.onClick = [this]
    {
        processorRef.presetManager.loadPrevious();
        refreshPresetBox();
    };

    nextPresetButton.onClick = [this]
    {
        processorRef.presetManager.loadNext();
        refreshPresetBox();
    };

    savePresetButton.onClick = [this]
    {
        auto& manager = processorRef.presetManager;
        const auto suggested = manager.getUserPresetDirectory()
                                   .getChildFile (manager.getCurrentName() + ".xml");

        fileChooser = std::make_unique<juce::FileChooser> ("Save preset", suggested, "*.xml");

        fileChooser->launchAsync (juce::FileBrowserComponent::saveMode
                                    | juce::FileBrowserComponent::canSelectFiles
                                    | juce::FileBrowserComponent::warnAboutOverwriting,
                                  [this] (const juce::FileChooser& fc)
                                  {
                                      const auto file = fc.getResult();

                                      if (file == juce::File{})
                                          return;

                                      processorRef.presetManager.saveUserPreset (file);
                                      refreshPresetBox();
                                  });
    };

    loadTableButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser> ("Load wavetable (.wav of 2048-sample frames)",
                                                           juce::File{}, "*.wav");

        fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                                    | juce::FileBrowserComponent::canSelectFiles,
                                  [this] (const juce::FileChooser& fc)
                                  {
                                      const auto file = fc.getResult();

                                      if (file == juce::File{})
                                          return;

                                      const auto error = processorRef.loadUserWavetable (file);

                                      if (error.isNotEmpty())
                                      {
                                          juce::AlertWindow::showMessageBoxAsync (
                                              juce::MessageBoxIconType::WarningIcon,
                                              "Wavetable import", error);
                                          return;
                                      }

                                      if (auto* param = processorRef.apvts.getParameter ("oscATable"))
                                          param->setValueNotifyingHost (param->convertTo0to1 (3.0f));
                                  });
    };

    // --- visualizer -------------------------------------------------------
    addAndMakeVisible (visualizer);

    // --- synth engine (left column) ---------------------------------------
    addAndMakeVisible (wtDisplayA);
    addAndMakeVisible (wtDisplayB);

    setupOscRow (oscARow, "oscA");
    setupOscRow (oscBRow, "oscB");
    oscARow.pos.setModTarget (SynthVoice::tgtOscAPos);
    oscARow.level.setModTarget (SynthVoice::tgtOscALevel);
    oscBRow.pos.setModTarget (SynthVoice::tgtOscBPos);
    oscBRow.level.setModTarget (SynthVoice::tgtOscBLevel);

    setupRotary (cutoffSlider, cutoffLabel, "Cutoff");
    setupRotary (resonanceSlider, resonanceLabel, "Resonance");
    setupRotary (morphSlider, morphLabel, "Morph");
    setupRotary (driveSlider, driveLabel, "Drive");
    setupRotary (keytrackSlider, keytrackLabel, "Keytrack");
    setupRotary (envAmtSlider, envAmtLabel, "Env Amt");
    cutoffSlider.setModTarget (SynthVoice::tgtCutoff);
    resonanceSlider.setModTarget (SynthVoice::tgtResonance);
    morphSlider.setModTarget (SynthVoice::tgtFilterMorph);

    setupCombo (subOctBox, subOctLabel, "Sub Oct", juce::StringArray { "-1 Oct", "-2 Oct" });
    setupRotary (subLevelSlider, subLevelLabel, "Sub Level");
    setupRotary (noiseSlider, noiseLabel, "Noise");
    setupRotary (attackSlider, attackLabel, "Attack");
    setupRotary (decaySlider, decayLabel, "Decay");
    setupRotary (sustainSlider, sustainLabel, "Sustain");
    setupRotary (releaseSlider, releaseLabel, "Release");
    setupRotary (gainSlider, gainLabel, "Gain");

    setupCombo (voiceModeBox, voiceModeLabel, "Voice", juce::StringArray { "Poly", "Mono", "Legato" });
    setupRotary (glideSlider, glideLabel, "Glide");
    setupRotary (bendRangeSlider, bendRangeLabel, "Bend Range");
    addAndMakeVisible (pumpOnButton);
    addAndMakeVisible (pumpSyncButton);
    setupCombo (pumpDivBox, pumpDivLabel, "Division", TempoDivisions::names());
    setupRotary (pumpAmountSlider, pumpAmountLabel, "Amount");
    setupRotary (pumpRateSlider, pumpRateLabel, "Rate");

    // --- modulation + FX (right column) ------------------------------------
    const juce::StringArray lfoShapes { "Sine", "Triangle", "Saw Down", "Square", "S&H" };
    setupCombo (lfo1ShapeBox, lfo1ShapeLabel, "LFO 1", lfoShapes);
    setupCombo (lfo2ShapeBox, lfo2ShapeLabel, "LFO 2", lfoShapes);

    for (auto* button : { &lfo1SyncButton, &lfo1FreeButton, &lfo2SyncButton, &lfo2FreeButton })
        addAndMakeVisible (*button);

    setupCombo (lfo1DivBox, lfo1DivLabel, "Division", TempoDivisions::names());
    setupCombo (lfo2DivBox, lfo2DivLabel, "Division", TempoDivisions::names());
    setupRotary (lfo1RateSlider, lfo1RateLabel, "Rate");
    setupRotary (lfo2RateSlider, lfo2RateLabel, "Rate");

    setupRotary (env2AttackSlider, env2AttackLabel, "Attack");
    setupRotary (env2DecaySlider, env2DecayLabel, "Decay");
    setupRotary (env2SustainSlider, env2SustainLabel, "Sustain");
    setupRotary (env2ReleaseSlider, env2ReleaseLabel, "Release");

    for (auto* chip : { &chipLFO1, &chipLFO2, &chipEnv2, &chipVel, &chipWheel })
        addAndMakeVisible (*chip);

    for (int s = 0; s < SynthVoice::numModSlots; ++s)
        setupModSlot (modSlots[s], s + 1);

    for (auto* button : { &distOnButton, &crushOnButton, &phaserOnButton,
                          &chorusOnButton, &delayOnButton, &delaySyncButton, &reverbOnButton })
        addAndMakeVisible (*button);

    setupCombo (delayDivBox, delayDivLabel, "Division", TempoDivisions::names());

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

    // --- drag-and-drop routing ----------------------------------------------
    const auto dropHandler = [this] (int source, int target) { assignModRouting (source, target); };

    for (auto* target : { &oscARow.pos, &oscARow.level, &oscBRow.pos, &oscBRow.level,
                          &cutoffSlider, &resonanceSlider, &morphSlider })
        target->onModDrop = dropHandler;

    // --- attachments --------------------------------------------------------
    auto& apvts = processorRef.apvts;

    cutoffAttachment    = std::make_unique<SliderAttachment> (apvts, "filterCutoff", cutoffSlider);
    resonanceAttachment = std::make_unique<SliderAttachment> (apvts, "filterRes", resonanceSlider);
    morphAttachment     = std::make_unique<SliderAttachment> (apvts, "filterMorph", morphSlider);
    driveAttachment     = std::make_unique<SliderAttachment> (apvts, "filterDrive", driveSlider);
    keytrackAttachment  = std::make_unique<SliderAttachment> (apvts, "filterKeytrack", keytrackSlider);
    envAmtAttachment    = std::make_unique<SliderAttachment> (apvts, "filterEnvAmt", envAmtSlider);

    subOctAttachment    = std::make_unique<ComboBoxAttachment> (apvts, "subOctave", subOctBox);
    subLevelAttachment  = std::make_unique<SliderAttachment> (apvts, "subLevel", subLevelSlider);
    noiseAttachment     = std::make_unique<SliderAttachment> (apvts, "noiseLevel", noiseSlider);
    attackAttachment    = std::make_unique<SliderAttachment> (apvts, "attack", attackSlider);
    decayAttachment     = std::make_unique<SliderAttachment> (apvts, "decay", decaySlider);
    sustainAttachment   = std::make_unique<SliderAttachment> (apvts, "sustain", sustainSlider);
    releaseAttachment   = std::make_unique<SliderAttachment> (apvts, "release", releaseSlider);
    gainAttachment      = std::make_unique<SliderAttachment> (apvts, "gain", gainSlider);

    voiceModeAttachment = std::make_unique<ComboBoxAttachment> (apvts, "voiceMode", voiceModeBox);
    glideAttachment     = std::make_unique<SliderAttachment> (apvts, "glideTime", glideSlider);
    bendRangeAttachment = std::make_unique<SliderAttachment> (apvts, "pitchBendRange", bendRangeSlider);
    pumpOnAtt           = std::make_unique<ButtonAttachment> (apvts, "pumpOn", pumpOnButton);
    pumpSyncAtt         = std::make_unique<ButtonAttachment> (apvts, "pumpSync", pumpSyncButton);
    pumpDivAtt          = std::make_unique<ComboBoxAttachment> (apvts, "pumpDiv", pumpDivBox);
    pumpAmountAtt       = std::make_unique<SliderAttachment> (apvts, "pumpAmount", pumpAmountSlider);
    pumpRateAtt         = std::make_unique<SliderAttachment> (apvts, "pumpRate", pumpRateSlider);

    lfo1ShapeAttachment = std::make_unique<ComboBoxAttachment> (apvts, "lfo1Shape", lfo1ShapeBox);
    lfo2ShapeAttachment = std::make_unique<ComboBoxAttachment> (apvts, "lfo2Shape", lfo2ShapeBox);
    lfo1RateAttachment  = std::make_unique<SliderAttachment> (apvts, "lfo1Rate", lfo1RateSlider);
    lfo2RateAttachment  = std::make_unique<SliderAttachment> (apvts, "lfo2Rate", lfo2RateSlider);
    lfo1SyncAtt  = std::make_unique<ButtonAttachment> (apvts, "lfo1Sync", lfo1SyncButton);
    lfo1FreeAtt  = std::make_unique<ButtonAttachment> (apvts, "lfo1Free", lfo1FreeButton);
    lfo2SyncAtt  = std::make_unique<ButtonAttachment> (apvts, "lfo2Sync", lfo2SyncButton);
    lfo2FreeAtt  = std::make_unique<ButtonAttachment> (apvts, "lfo2Free", lfo2FreeButton);
    lfo1DivAtt   = std::make_unique<ComboBoxAttachment> (apvts, "lfo1Div", lfo1DivBox);
    lfo2DivAtt   = std::make_unique<ComboBoxAttachment> (apvts, "lfo2Div", lfo2DivBox);

    env2AttackAttachment  = std::make_unique<SliderAttachment> (apvts, "env2Attack", env2AttackSlider);
    env2DecayAttachment   = std::make_unique<SliderAttachment> (apvts, "env2Decay", env2DecaySlider);
    env2SustainAttachment = std::make_unique<SliderAttachment> (apvts, "env2Sustain", env2SustainSlider);
    env2ReleaseAttachment = std::make_unique<SliderAttachment> (apvts, "env2Release", env2ReleaseSlider);

    distOnAtt    = std::make_unique<ButtonAttachment> (apvts, "distOn", distOnButton);
    crushOnAtt   = std::make_unique<ButtonAttachment> (apvts, "crushOn", crushOnButton);
    phaserOnAtt  = std::make_unique<ButtonAttachment> (apvts, "phaserOn", phaserOnButton);
    chorusOnAtt  = std::make_unique<ButtonAttachment> (apvts, "chorusOn", chorusOnButton);
    delayOnAtt   = std::make_unique<ButtonAttachment> (apvts, "delayOn", delayOnButton);
    delaySyncAtt = std::make_unique<ButtonAttachment> (apvts, "delaySync", delaySyncButton);
    reverbOnAtt  = std::make_unique<ButtonAttachment> (apvts, "reverbOn", reverbOnButton);
    delayDivAtt  = std::make_unique<ComboBoxAttachment> (apvts, "delayDiv", delayDivBox);

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

    // --- typing keyboard (FL Studio-style) ---------------------------------
    setWantsKeyboardFocus (true);
    initTypingKeys();
    keyboard.clearKeyMappings();        // the editor handles typing itself
    keyboard.setOctaveForMiddleC (5);   // FL-style naming: middle C = C5

    addAndMakeVisible (octDownButton);
    addAndMakeVisible (octUpButton);
    addAndMakeVisible (octLabel);
    octLabel.setJustificationType (juce::Justification::centred);
    setKeyboardOctave (4);

    octDownButton.onClick = [this] { setKeyboardOctave (keyboardOctave - 1); };
    octUpButton.onClick   = [this] { setKeyboardOctave (keyboardOctave + 1); };

    // Readable value boxes. This has to come after the attachments are made:
    // SliderAttachment installs its own textFromValueFunction, which would
    // otherwise win and print every value to 7 decimal places.
    for (auto* child : getChildren())
    {
        auto* slider = dynamic_cast<juce::Slider*> (child);

        if (slider == nullptr || slider->getInterval() > 0.0)
            continue;   // stepped sliders already read as whole numbers

        slider->textFromValueFunction = [] (double v)
        {
            const double mag = std::abs (v);

            if (mag >= 100.0) return juce::String (juce::roundToInt (v));
            if (mag >= 10.0)  return juce::String (v, 1);

            return juce::String (v, 2);
        };

        slider->updateText();
    }

    addAndMakeVisible (keyboard);

    setSize (1460, 900);
}

void AstralVegaAudioProcessorEditor::initTypingKeys()
{
    // FL Studio layout: Z row = base octave, Q row = one octave up
    static constexpr struct { char key; int offset; } layout[] = {
        { 'z', 0 },  { 's', 1 },  { 'x', 2 },  { 'd', 3 },  { 'c', 4 },
        { 'v', 5 },  { 'g', 6 },  { 'b', 7 },  { 'h', 8 },  { 'n', 9 },
        { 'j', 10 }, { 'm', 11 }, { ',', 12 }, { 'l', 13 }, { '.', 14 },
        { ';', 15 }, { '/', 16 },

        { 'q', 12 }, { '2', 13 }, { 'w', 14 }, { '3', 15 }, { 'e', 16 },
        { 'r', 17 }, { '5', 18 }, { 't', 19 }, { '6', 20 }, { 'y', 21 },
        { '7', 22 }, { 'u', 23 }, { 'i', 24 }, { '9', 25 }, { 'o', 26 },
        { '0', 27 }, { 'p', 28 },
    };

    for (const auto& k : layout)
        typingKeys.push_back ({ (int) k.key, k.offset, -1 });
}

void AstralVegaAudioProcessorEditor::setKeyboardOctave (int newOctave)
{
    keyboardOctave = juce::jlimit (0, 8, newOctave);
    octLabel.setText ("C" + juce::String (keyboardOctave), juce::dontSendNotification);
}

bool AstralVegaAudioProcessorEditor::keyStateChanged (bool)
{
    // don't steal keystrokes while the user is typing into a value box
    if (dynamic_cast<juce::TextEditor*> (juce::Component::getCurrentlyFocusedComponent()) != nullptr)
        return false;

    bool handled = false;

    for (auto& key : typingKeys)
    {
        const bool down = juce::KeyPress::isKeyCurrentlyDown (key.keyCode);

        if (down && key.playingNote < 0)
        {
            const int note = 12 * keyboardOctave + key.offset;

            if (note < 128)
            {
                processorRef.keyboardState.noteOn (1, note, 0.8f);
                key.playingNote = note;   // remember the sounding note so an
                handled = true;           // octave change can't strand it
            }
        }
        else if (! down && key.playingNote >= 0)
        {
            processorRef.keyboardState.noteOff (1, key.playingNote, 0.8f);
            key.playingNote = -1;
            handled = true;
        }
    }

    return handled;
}

void AstralVegaAudioProcessorEditor::releaseAllTypingNotes()
{
    for (auto& key : typingKeys)
    {
        if (key.playingNote >= 0)
        {
            processorRef.keyboardState.noteOff (1, key.playingNote, 0.0f);
            key.playingNote = -1;
        }
    }
}

void AstralVegaAudioProcessorEditor::focusLost (FocusChangeType)
{
    if (! hasKeyboardFocus (true))
        releaseAllTypingNotes();
}

void AstralVegaAudioProcessorEditor::focusOfChildComponentChanged (FocusChangeType)
{
    if (! hasKeyboardFocus (true))
        releaseAllTypingNotes();
}

AstralVegaAudioProcessorEditor::~AstralVegaAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================

void AstralVegaAudioProcessorEditor::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    presetBox.addItemList (processorRef.presetManager.getPresetNames(), 1);
    presetBox.setSelectedItemIndex (processorRef.presetManager.getCurrentIndex(),
                                    juce::dontSendNotification);
}

void AstralVegaAudioProcessorEditor::assignModRouting (int sourceIndex, int targetIndex)
{
    auto& apvts = processorRef.apvts;

    for (int s = 1; s <= SynthVoice::numModSlots; ++s)
    {
        const auto prefix = "mod" + juce::String (s);
        const int slotSrc = (int) apvts.getRawParameterValue (prefix + "Src")->load();
        const int slotDst = (int) apvts.getRawParameterValue (prefix + "Dst")->load();

        const bool sameRouting = slotSrc == sourceIndex && slotDst == targetIndex;
        const bool freeSlot = slotSrc == SynthVoice::srcNone || slotDst == SynthVoice::tgtNone;

        if (! sameRouting && ! freeSlot)
            continue;

        auto* srcParam = apvts.getParameter (prefix + "Src");
        auto* dstParam = apvts.getParameter (prefix + "Dst");
        auto* amtParam = apvts.getParameter (prefix + "Amt");

        srcParam->setValueNotifyingHost (srcParam->convertTo0to1 ((float) sourceIndex));
        dstParam->setValueNotifyingHost (dstParam->convertTo0to1 ((float) targetIndex));

        if (std::abs (apvts.getRawParameterValue (prefix + "Amt")->load()) < 0.01f)
            amtParam->setValueNotifyingHost (amtParam->convertTo0to1 (0.5f));

        return;
    }

    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon,
                                            "Mod matrix",
                                            "All 6 matrix slots are in use — clear one first.");
}

void AstralVegaAudioProcessorEditor::setupRotary (juce::Slider& slider, juce::Label& label,
                                                  const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 13);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.attachToComponent (&slider, false);
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

void AstralVegaAudioProcessorEditor::setupOscRow (OscRowControls& row, const juce::String& idPrefix)
{
    setupCombo (row.table, row.tableLabel, "Table",
                juce::StringArray { "Basic", "PWM", "Spectra", "User" });

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

//==============================================================================

void AstralVegaAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (AstralLookAndFeel::backgroundHi, 0.0f, 0.0f,
                                             AstralLookAndFeel::background,
                                             0.0f, (float) getHeight(), false));
    g.fillAll();

    for (const auto& panel : panels)
    {
        const auto area = panel.area.toFloat();

        g.setColour (AstralLookAndFeel::deepPurple.withAlpha (0.35f));
        g.fillRoundedRectangle (area, 6.0f);
        g.setColour (AstralLookAndFeel::panelOutline.withAlpha (0.5f));
        g.drawRoundedRectangle (area.reduced (0.5f), 6.0f, 1.0f);

        g.setColour (AstralLookAndFeel::cyan.withAlpha (0.55f));
        g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
        g.drawText (panel.title, panel.area.reduced (10, 4),
                    juce::Justification::topLeft);
    }

    auto titleArea = getLocalBounds().removeFromTop (48).toFloat();
    g.setColour (AstralLookAndFeel::magenta);
    g.setFont (AstralLookAndFeel::getTitleFont (24.0f));
    g.drawText ("ASTRAL VEGA", titleArea.withTrimmedLeft (24.0f),
                juce::Justification::centredLeft);

    g.setColour (AstralLookAndFeel::cyan.withAlpha (0.6f));
    g.drawLine (16.0f, titleArea.getBottom(),
                (float) getWidth() - 16.0f, titleArea.getBottom(), 1.5f);
}

juce::Rectangle<int> AstralVegaAudioProcessorEditor::addPanel (juce::Rectangle<int> area,
                                                               const juce::String& title)
{
    panels.push_back ({ area, title });

    auto content = area.reduced (8, 3);
    content.removeFromTop (10);
    return content;
}

void AstralVegaAudioProcessorEditor::resized()
{
    panels.clear();

    auto bounds = getLocalBounds().reduced (16);

    // octave strip + keyboard along the bottom
    auto keyboardArea = bounds.removeFromBottom (80);
    auto octArea = keyboardArea.removeFromLeft (104).withSizeKeepingCentre (104, 34);
    octDownButton.setBounds (octArea.removeFromLeft (32));
    octLabel.setBounds (octArea.removeFromLeft (36));
    octUpButton.setBounds (octArea);
    keyboardArea.removeFromLeft (6);
    keyboard.setBounds (keyboardArea);

    bounds.removeFromBottom (6);

    // title strip: name painted on the left, preset bar on the right
    auto titleStrip = bounds.removeFromTop (48);
    auto presetArea = titleStrip.removeFromRight (580).reduced (0, 10);
    prevPresetButton.setBounds (presetArea.removeFromLeft (30));
    presetArea.removeFromLeft (4);
    presetBox.setBounds (presetArea.removeFromLeft (300));
    presetArea.removeFromLeft (4);
    nextPresetButton.setBounds (presetArea.removeFromLeft (30));
    presetArea.removeFromLeft (10);
    savePresetButton.setBounds (presetArea.removeFromLeft (70));
    presetArea.removeFromLeft (10);
    loadTableButton.setBounds (presetArea);

    bounds.removeFromTop (6);
    visualizer.setBounds (bounds.removeFromTop (92));
    bounds.removeFromTop (8);

    // left column: the synth engine; right column: modulation + FX rack
    auto left = bounds.removeFromLeft (juce::roundToInt ((float) bounds.getWidth() * 0.56f));
    auto right = bounds.withTrimmedLeft (12);

    const int leftRowH = left.getHeight() / 5;

    // each osc panel carries its wavetable display on the left; the knobs are
    // height-limited, so the narrower cells don't shrink them
    const auto layoutOscRow = [this] (juce::Rectangle<int> content,
                                      WavetableDisplay& display, OscRowControls& row)
    {
        display.setBounds (content.removeFromLeft (168).reduced (2, 1));
        content.removeFromLeft (6);

        layoutRow (content, { &row.table, &row.pos, &row.coarse, &row.unison,
                              &row.detune, &row.spread, &row.level });
    };

    layoutOscRow (addPanel (left.removeFromTop (leftRowH).reduced (0, 2), "OSC A"),
                  wtDisplayA, oscARow);

    layoutOscRow (addPanel (left.removeFromTop (leftRowH).reduced (0, 2), "OSC B"),
                  wtDisplayB, oscBRow);

    layoutRow (addPanel (left.removeFromTop (leftRowH).reduced (0, 2), "FILTER"),
               { &cutoffSlider, &resonanceSlider, &morphSlider,
                 &driveSlider, &keytrackSlider, &envAmtSlider });

    layoutRow (addPanel (left.removeFromTop (leftRowH).reduced (0, 2), "MIX / AMP ENV"),
               { &subOctBox, &subLevelSlider, &noiseSlider, &attackSlider,
                 &decaySlider, &sustainSlider, &releaseSlider, &gainSlider });

    layoutRow (addPanel (left.reduced (0, 2), "PERFORM / PUMP"),
               { &voiceModeBox, &glideSlider, &bendRangeSlider, &pumpOnButton,
                 &pumpSyncButton, &pumpDivBox, &pumpAmountSlider, &pumpRateSlider });

    const int rightH = right.getHeight();
    const int unitH = rightH * 14 / 100;

    layoutRow (addPanel (right.removeFromTop (unitH).reduced (0, 2), "LFO 1 / LFO 2"),
               { &lfo1ShapeBox, &lfo1FreeButton, &lfo1SyncButton, &lfo1DivBox, &lfo1RateSlider,
                 &lfo2ShapeBox, &lfo2FreeButton, &lfo2SyncButton, &lfo2DivBox, &lfo2RateSlider });

    layoutRow (addPanel (right.removeFromTop (unitH).reduced (0, 2), "ENV 2"),
               { &env2AttackSlider, &env2DecaySlider, &env2SustainSlider, &env2ReleaseSlider });

    // matrix panel: drag chips strip on top, then two rows of three slots
    auto matrixContent = addPanel (right.removeFromTop (rightH * 30 / 100).reduced (0, 2),
                                   "MOD MATRIX - drag a chip onto a glowing knob");

    auto chipStrip = matrixContent.removeFromTop (22);
    const int chipW = chipStrip.getWidth() / 5;

    for (auto* chip : { &chipLFO1, &chipLFO2, &chipEnv2, &chipVel, &chipWheel })
        chip->setBounds (chipStrip.removeFromLeft (chipW).reduced (4, 0));

    const int matrixRowH = matrixContent.getHeight() / 2;

    layoutRow (matrixContent.removeFromTop (matrixRowH),
               { &modSlots[0].src, &modSlots[0].dst, &modSlots[0].amt,
                 &modSlots[1].src, &modSlots[1].dst, &modSlots[1].amt,
                 &modSlots[2].src, &modSlots[2].dst, &modSlots[2].amt });

    layoutRow (matrixContent,
               { &modSlots[3].src, &modSlots[3].dst, &modSlots[3].amt,
                 &modSlots[4].src, &modSlots[4].dst, &modSlots[4].amt,
                 &modSlots[5].src, &modSlots[5].dst, &modSlots[5].amt });

    layoutRow (addPanel (right.removeFromTop (unitH).reduced (0, 2), "DIST / CRUSH"),
               { &distOnButton, &distDriveSlider, &distMixSlider,
                 &crushOnButton, &crushBitsSlider, &crushRateSlider });

    layoutRow (addPanel (right.removeFromTop (unitH).reduced (0, 2), "PHASER / CHORUS"),
               { &phaserOnButton, &phaserRateSlider, &phaserDepthSlider, &phaserMixSlider,
                 &chorusOnButton, &chorusRateSlider, &chorusDepthSlider, &chorusMixSlider });

    layoutRow (addPanel (right.reduced (0, 2), "DELAY / REVERB"),
               { &delayOnButton, &delaySyncButton, &delayDivBox,
                 &delayTimeSlider, &delayFBSlider, &delayMixSlider,
                 &reverbOnButton, &reverbSizeSlider, &reverbDampSlider, &reverbMixSlider });
}
