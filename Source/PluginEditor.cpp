#include "PluginEditor.h"

namespace
{
    const juce::Colour background { 0xff14101f };
    const juce::Colour neonMagenta { 0xffff2d95 };
    const juce::Colour neonCyan { 0xff29e6ff };
}

AstralVegaAudioProcessorEditor::AstralVegaAudioProcessorEditor (AstralVegaAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    tableBox.addItemList (juce::StringArray { "Basic", "PWM", "Spectra" }, 1);
    addAndMakeVisible (tableBox);
    tableLabel.setText ("Wavetable", juce::dontSendNotification);
    tableLabel.setJustificationType (juce::Justification::centred);
    tableLabel.attachToComponent (&tableBox, false);

    setupRotary (posSlider, posLabel, "WT Pos");
    setupRotary (unisonSlider, unisonLabel, "Unison");
    setupRotary (detuneSlider, detuneLabel, "Detune");
    setupRotary (spreadSlider, spreadLabel, "Spread");
    setupRotary (cutoffSlider, cutoffLabel, "Cutoff");
    setupRotary (resonanceSlider, resonanceLabel, "Resonance");
    setupRotary (gainSlider, gainLabel, "Gain");
    setupRotary (attackSlider, attackLabel, "Attack");
    setupRotary (decaySlider, decayLabel, "Decay");
    setupRotary (sustainSlider, sustainLabel, "Sustain");
    setupRotary (releaseSlider, releaseLabel, "Release");

    auto& apvts = processorRef.apvts;
    tableAttachment     = std::make_unique<ComboBoxAttachment> (apvts, "oscTable", tableBox);
    posAttachment       = std::make_unique<SliderAttachment> (apvts, "oscPos", posSlider);
    unisonAttachment    = std::make_unique<SliderAttachment> (apvts, "unison", unisonSlider);
    detuneAttachment    = std::make_unique<SliderAttachment> (apvts, "detune", detuneSlider);
    spreadAttachment    = std::make_unique<SliderAttachment> (apvts, "spread", spreadSlider);
    cutoffAttachment    = std::make_unique<SliderAttachment> (apvts, "filterCutoff", cutoffSlider);
    resonanceAttachment = std::make_unique<SliderAttachment> (apvts, "filterRes", resonanceSlider);
    gainAttachment      = std::make_unique<SliderAttachment> (apvts, "gain", gainSlider);
    attackAttachment    = std::make_unique<SliderAttachment> (apvts, "attack", attackSlider);
    decayAttachment     = std::make_unique<SliderAttachment> (apvts, "decay", decaySlider);
    sustainAttachment   = std::make_unique<SliderAttachment> (apvts, "sustain", sustainSlider);
    releaseAttachment   = std::make_unique<SliderAttachment> (apvts, "release", releaseSlider);

    addAndMakeVisible (keyboard);

    setSize (980, 500);
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

    // row 1: oscillator section
    auto row1 = bounds.removeFromTop (bounds.getHeight() / 2);
    row1.removeFromTop (20); // room for attached labels
    auto cellW = row1.getWidth() / 5;

    auto tableCell = row1.removeFromLeft (cellW);
    tableBox.setBounds (tableCell.withSizeKeepingCentre (cellW - 32, 28));
    posSlider.setBounds (row1.removeFromLeft (cellW).reduced (8));
    unisonSlider.setBounds (row1.removeFromLeft (cellW).reduced (8));
    detuneSlider.setBounds (row1.removeFromLeft (cellW).reduced (8));
    spreadSlider.setBounds (row1.reduced (8));

    // row 2: filter, envelope, output
    auto row2 = bounds;
    row2.removeFromTop (20);
    cellW = row2.getWidth() / 7;
    cutoffSlider.setBounds (row2.removeFromLeft (cellW).reduced (8));
    resonanceSlider.setBounds (row2.removeFromLeft (cellW).reduced (8));
    attackSlider.setBounds (row2.removeFromLeft (cellW).reduced (8));
    decaySlider.setBounds (row2.removeFromLeft (cellW).reduced (8));
    sustainSlider.setBounds (row2.removeFromLeft (cellW).reduced (8));
    releaseSlider.setBounds (row2.removeFromLeft (cellW).reduced (8));
    gainSlider.setBounds (row2.reduced (8));
}
