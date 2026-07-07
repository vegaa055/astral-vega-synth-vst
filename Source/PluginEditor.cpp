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
                comp->setBounds (cell.withSizeKeepingCentre (cellW - 32, 28));
            else
                comp->setBounds (cell.reduced (8));
        }
    }
}

AstralVegaAudioProcessorEditor::AstralVegaAudioProcessorEditor (AstralVegaAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setupOscRow (oscARow, "oscA", "Osc A");
    setupOscRow (oscBRow, "oscB", "Osc B");

    subOctBox.addItemList (juce::StringArray { "-1 Oct", "-2 Oct" }, 1);
    addAndMakeVisible (subOctBox);
    subOctLabel.setText ("Sub Oct", juce::dontSendNotification);
    subOctLabel.setJustificationType (juce::Justification::centred);
    subOctLabel.attachToComponent (&subOctBox, false);

    setupRotary (subLevelSlider, subLevelLabel, "Sub Level");
    setupRotary (noiseSlider, noiseLabel, "Noise");
    setupRotary (cutoffSlider, cutoffLabel, "Cutoff");
    setupRotary (resonanceSlider, resonanceLabel, "Resonance");
    setupRotary (gainSlider, gainLabel, "Gain");
    setupRotary (attackSlider, attackLabel, "Attack");
    setupRotary (decaySlider, decayLabel, "Decay");
    setupRotary (sustainSlider, sustainLabel, "Sustain");
    setupRotary (releaseSlider, releaseLabel, "Release");

    auto& apvts = processorRef.apvts;
    subOctAttachment    = std::make_unique<ComboBoxAttachment> (apvts, "subOctave", subOctBox);
    subLevelAttachment  = std::make_unique<SliderAttachment> (apvts, "subLevel", subLevelSlider);
    noiseAttachment     = std::make_unique<SliderAttachment> (apvts, "noiseLevel", noiseSlider);
    cutoffAttachment    = std::make_unique<SliderAttachment> (apvts, "filterCutoff", cutoffSlider);
    resonanceAttachment = std::make_unique<SliderAttachment> (apvts, "filterRes", resonanceSlider);
    gainAttachment      = std::make_unique<SliderAttachment> (apvts, "gain", gainSlider);
    attackAttachment    = std::make_unique<SliderAttachment> (apvts, "attack", attackSlider);
    decayAttachment     = std::make_unique<SliderAttachment> (apvts, "decay", decaySlider);
    sustainAttachment   = std::make_unique<SliderAttachment> (apvts, "sustain", sustainSlider);
    releaseAttachment   = std::make_unique<SliderAttachment> (apvts, "release", releaseSlider);

    addAndMakeVisible (keyboard);

    setSize (1000, 620);
}

void AstralVegaAudioProcessorEditor::setupRotary (juce::Slider& slider, juce::Label& label,
                                                  const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
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

    keyboard.setBounds (bounds.removeFromBottom (96));
    bounds.removeFromTop (48); // title strip painted in paint()

    const int rowH = bounds.getHeight() / 4;

    layoutRow (bounds.removeFromTop (rowH),
               { &oscARow.table, &oscARow.pos, &oscARow.coarse, &oscARow.unison,
                 &oscARow.detune, &oscARow.spread, &oscARow.level });

    layoutRow (bounds.removeFromTop (rowH),
               { &oscBRow.table, &oscBRow.pos, &oscBRow.coarse, &oscBRow.unison,
                 &oscBRow.detune, &oscBRow.spread, &oscBRow.level });

    layoutRow (bounds.removeFromTop (rowH),
               { &subOctBox, &subLevelSlider, &noiseSlider,
                 &cutoffSlider, &resonanceSlider, &gainSlider });

    layoutRow (bounds,
               { &attackSlider, &decaySlider, &sustainSlider, &releaseSlider });
}
