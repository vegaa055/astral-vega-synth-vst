#pragma once

#include <JuceHeader.h>

/** The Astral Vega skin: neon knobs with glowing value arcs, LED-style
    toggles, and a shared synthwave palette used across the editor. */
class AstralLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AstralLookAndFeel();

    inline static const juce::Colour background   { 0xff0e0a1a };
    inline static const juce::Colour backgroundHi { 0xff191030 };
    inline static const juce::Colour deepPurple   { 0xff231640 };
    inline static const juce::Colour panel        { 0x2c9a7bff };
    inline static const juce::Colour panelOutline { 0x5529e6ff };
    inline static const juce::Colour textDim      { 0xffb8aed6 };
    inline static const juce::Colour textBright   { 0xfff2eefc };
    inline static const juce::Colour magenta      { 0xffff2d95 };
    inline static const juce::Colour cyan         { 0xff29e6ff };

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont (juce::Label&) override;
};
