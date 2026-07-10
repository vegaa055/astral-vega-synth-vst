#include "AstralLookAndFeel.h"
#include "BinaryData.h"

juce::Font AstralLookAndFeel::getTitleFont (float height)
{
    static const juce::Typeface::Ptr abnes =
        juce::Typeface::createSystemTypefaceFor (BinaryData::abnes_ttf,
                                                 (size_t) BinaryData::abnes_ttfSize);

    if (abnes != nullptr)
        return juce::Font (juce::FontOptions (abnes).withHeight (height));

    return juce::Font (juce::FontOptions (height, juce::Font::bold));
}

AstralLookAndFeel::AstralLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, background);

    setColour (juce::Slider::textBoxTextColourId, textDim);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::rotarySliderFillColourId, magenta);
    setColour (juce::Slider::thumbColourId, cyan);

    setColour (juce::Label::textColourId, textDim);

    setColour (juce::ComboBox::backgroundColourId, deepPurple);
    setColour (juce::ComboBox::outlineColourId, panelOutline);
    setColour (juce::ComboBox::textColourId, textBright);
    setColour (juce::ComboBox::arrowColourId, cyan);

    setColour (juce::PopupMenu::backgroundColourId, deepPurple);
    setColour (juce::PopupMenu::textColourId, textBright);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, magenta.withAlpha (0.35f));
    setColour (juce::PopupMenu::highlightedTextColourId, textBright);

    setColour (juce::TextButton::buttonColourId, deepPurple);
    setColour (juce::TextButton::buttonOnColourId, magenta.withAlpha (0.5f));
    setColour (juce::TextButton::textColourOffId, textBright);
    setColour (juce::TextButton::textColourOnId, textBright);

    setColour (juce::ToggleButton::textColourId, textDim);
    setColour (juce::ToggleButton::tickColourId, magenta);

    setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffe6e1f2));
    setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff1a1030));
    setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff2a2145));
    setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, magenta.withAlpha (0.6f));
    setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, cyan.withAlpha (0.3f));
    setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);

    setColour (juce::AlertWindow::backgroundColourId, deepPurple);
    setColour (juce::AlertWindow::textColourId, textBright);
}

void AstralLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float rotaryStartAngle,
                                          float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y,
                                          (float) width, (float) height).reduced (5.0f);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    bounds = bounds.withSizeKeepingCentre (size, size);

    const auto centre = bounds.getCentre();
    const float radius = size * 0.5f;
    const float arcRadius = radius - 2.0f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const auto fill = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const auto pointer = slider.findColour (juce::Slider::thumbColourId);

    const juce::PathStrokeType rounded (2.5f, juce::PathStrokeType::curved,
                                        juce::PathStrokeType::rounded);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius,
                         0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (deepPurple.brighter (0.25f));
    g.strokePath (track, rounded);

    if (sliderPos > 0.001f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius,
                             0.0f, rotaryStartAngle, angle, true);

        // soft glow pass under the crisp arc
        g.setColour (fill.withAlpha (0.35f));
        g.strokePath (value, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
        g.setColour (fill);
        g.strokePath (value, rounded);
    }

    const float bodyRadius = radius * 0.62f;
    const auto bodyBounds = juce::Rectangle<float> (bodyRadius * 2.0f,
                                                    bodyRadius * 2.0f).withCentre (centre);

    g.setGradientFill (juce::ColourGradient (deepPurple.brighter (0.3f),
                                             centre.x, centre.y - bodyRadius,
                                             background,
                                             centre.x, centre.y + bodyRadius, false));
    g.fillEllipse (bodyBounds);
    g.setColour (panelOutline);
    g.drawEllipse (bodyBounds, 1.0f);

    g.setColour (pointer);
    g.drawLine (juce::Line<float> (centre.getPointOnCircumference (bodyRadius * 0.3f, angle),
                                   centre.getPointOnCircumference (bodyRadius * 0.85f, angle)),
                2.5f);
}

void AstralLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                          bool shouldDrawButtonAsHighlighted,
                                          bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    const auto bounds = button.getLocalBounds().toFloat();
    const bool on = button.getToggleState();

    auto pill = bounds.withSizeKeepingCentre (juce::jmin (bounds.getWidth() - 2.0f, 82.0f),
                                              juce::jmin (bounds.getHeight(), 26.0f));

    g.setColour (on ? magenta.withAlpha (0.22f) : deepPurple);
    g.fillRoundedRectangle (pill, pill.getHeight() * 0.5f);
    g.setColour (on ? magenta : panelOutline);
    g.drawRoundedRectangle (pill, pill.getHeight() * 0.5f, on ? 1.5f : 1.0f);

    const auto led = juce::Rectangle<float> (7.0f, 7.0f)
                         .withCentre ({ pill.getX() + 10.0f, pill.getCentreY() });

    if (on)
    {
        g.setColour (magenta.withAlpha (0.4f));
        g.fillEllipse (led.expanded (3.0f));
    }

    g.setColour (on ? magenta : deepPurple.brighter (0.5f));
    g.fillEllipse (led);

    g.setColour (on ? textBright : textDim);
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.drawText (button.getButtonText(),
                pill.withTrimmedLeft (17.0f).withTrimmedRight (3.0f),
                juce::Justification::centredLeft);
}

juce::Font AstralLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (juce::FontOptions (12.0f));
}
