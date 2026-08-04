#include "WavetableDisplay.h"
#include "AstralLookAndFeel.h"

namespace
{
    /** Linear crossfade between adjacent frames, matching the audio engine. */
    forcedinline float sampleFrame (const AstralVegaAudioProcessor::DisplayTable& t,
                                    float framePos, int sampleIndex) noexcept
    {
        const int f0 = juce::jlimit (0, t.numFrames - 1, (int) framePos);
        const int f1 = juce::jmin (f0 + 1, t.numFrames - 1);
        const float frac = juce::jlimit (0.0f, 1.0f, framePos - (float) f0);

        const float a = t.frames[(size_t) f0 * (size_t) t.frameStride + (size_t) sampleIndex];
        const float b = t.frames[(size_t) f1 * (size_t) t.frameStride + (size_t) sampleIndex];

        return a + frac * (b - a);
    }
}

WavetableDisplay::WavetableDisplay (AstralVegaAudioProcessor& p, const juce::String& oscIdPrefix)
    : processor (p),
      tableParamId (oscIdPrefix + "Table"),
      posParamId (oscIdPrefix + "Pos")
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

void WavetableDisplay::timerCallback()
{
    const int tableIndex = (int) processor.apvts.getRawParameterValue (tableParamId)->load();
    const float position = processor.apvts.getRawParameterValue (posParamId)->load();

    if (tableIndex == lastTableIndex && juce::approximatelyEqual (position, lastPosition))
        return;

    lastTableIndex = tableIndex;
    lastPosition = position;
    repaint();
}

void WavetableDisplay::buildFramePath (juce::Path& path, const DisplayTable& table,
                                       float framePos, juce::Rectangle<float> area)
{
    const int columns = juce::jlimit (2, maxColumns, (int) area.getWidth());

    float his[maxColumns], los[maxColumns];
    float peak = 0.0f;

    // min/max per pixel column, so bright frames read as a solid band rather
    // than an aliased scribble
    for (int x = 0; x < columns; ++x)
    {
        const int s0 = x * table.frameLength / columns;
        const int s1 = juce::jmax (s0 + 1, (x + 1) * table.frameLength / columns);

        float lo = sampleFrame (table, framePos, s0);
        float hi = lo;

        for (int s = s0 + 1; s < s1; ++s)
        {
            const float v = sampleFrame (table, framePos, s);
            lo = juce::jmin (lo, v);
            hi = juce::jmax (hi, v);
        }

        his[x] = hi;
        los[x] = lo;
        peak = juce::jmax (peak, std::abs (hi), std::abs (lo));
    }

    // normalise per frame: the table itself is scaled by its loudest frame for
    // audio, which would otherwise draw darker frames as a near-flat line
    const float scale = area.getHeight() * 0.46f / juce::jmax (1.0e-4f, peak);
    const float centreY = area.getCentreY();

    for (int x = 0; x < columns; ++x)
    {
        const float px = area.getX() + area.getWidth() * (float) x / (float) (columns - 1);

        if (x == 0)
            path.startNewSubPath (px, centreY - his[x] * scale);
        else
            path.lineTo (px, centreY - his[x] * scale);

        path.lineTo (px, centreY - los[x] * scale);
    }
}

void WavetableDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (AstralLookAndFeel::background.brighter (0.04f));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (AstralLookAndFeel::panelOutline.withAlpha (0.45f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

    const int tableIndex = (int) processor.apvts.getRawParameterValue (tableParamId)->load();
    const auto table = processor.getDisplayTable (tableIndex);

    if (! table.isValid())
    {
        g.setColour (AstralLookAndFeel::textDim.withAlpha (0.45f));
        g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
        g.drawText ("LOAD WT", bounds, juce::Justification::centred);
        return;
    }

    const float position = juce::jlimit (0.0f, 1.0f,
                                         processor.apvts.getRawParameterValue (posParamId)->load());
    const float framePos = position * (float) (table.numFrames - 1);

    auto content = bounds.reduced (5.0f);
    auto cursorArea = content.removeFromBottom (10.0f);
    content.removeFromBottom (2.0f);

    // ghosts of frames from across the table, so you can see where a sweep goes
    if (table.numFrames > 1)
    {
        g.setColour (AstralLookAndFeel::cyan.withAlpha (0.12f));

        for (int k = 0; k < numGhostFrames; ++k)
        {
            const float ghostPos = (float) k / (float) (numGhostFrames - 1)
                                 * (float) (table.numFrames - 1);

            juce::Path ghost;
            buildFramePath (ghost, table, ghostPos, content);
            g.strokePath (ghost, juce::PathStrokeType (1.0f));
        }
    }

    juce::Path wave;
    buildFramePath (wave, table, framePos, content);

    g.setColour (AstralLookAndFeel::cyan.withAlpha (0.22f));
    g.strokePath (wave, juce::PathStrokeType (3.0f));
    g.setColour (AstralLookAndFeel::cyan);
    g.strokePath (wave, juce::PathStrokeType (1.3f));

    // position cursor
    auto track = cursorArea.withSizeKeepingCentre (cursorArea.getWidth(), 3.0f);
    g.setColour (AstralLookAndFeel::deepPurple.brighter (0.35f));
    g.fillRoundedRectangle (track, 1.5f);

    const float markerX = track.getX() + position * track.getWidth();
    const auto marker = juce::Rectangle<float> (3.0f, cursorArea.getHeight())
                            .withCentre ({ markerX, cursorArea.getCentreY() });

    g.setColour (AstralLookAndFeel::magenta.withAlpha (0.45f));
    g.fillRoundedRectangle (marker.expanded (2.0f, 1.0f), 2.0f);
    g.setColour (AstralLookAndFeel::magenta);
    g.fillRoundedRectangle (marker, 1.5f);

    g.setColour (AstralLookAndFeel::textDim.withAlpha (0.7f));
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.drawText (juce::String (framePos + 1.0f, 1) + " / " + juce::String (table.numFrames),
                bounds.reduced (7.0f, 4.0f), juce::Justification::topRight);
}
