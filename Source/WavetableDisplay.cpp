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

WavetableDisplay::WavetableDisplay (AstralVegaAudioProcessor& p,
                                    const juce::String& oscIdPrefix, int oscIndexIn)
    : processor (p),
      tableParamId (oscIdPrefix + "Table"),
      posParamId (oscIdPrefix + "Pos"),
      oscIndex (oscIndexIn)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

void WavetableDisplay::timerCallback()
{
    const int tableIndex = (int) processor.apvts.getRawParameterValue (tableParamId)->load();
    const float basePosition = processor.apvts.getRawParameterValue (posParamId)->load();

    float shown = basePosition;
    const bool live = processor.getLivePosition (oscIndex, shown);

    const bool changed = tableIndex != lastTableIndex
                      || live != lastLive
                      || std::abs (shown - lastShownPosition) > 0.0008f
                      || std::abs (basePosition - lastBasePosition) > 0.0008f;

    if (! changed)
        return;

    lastTableIndex = tableIndex;
    lastBasePosition = basePosition;
    lastShownPosition = shown;
    lastLive = live;
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

void WavetableDisplay::rebuildGhosts (const DisplayTable& table, juce::Rectangle<float> area)
{
    for (int k = 0; k < numGhostFrames; ++k)
    {
        ghostPaths[k].clear();

        if (table.numFrames > 1)
        {
            const float ghostPos = (float) k / (float) (numGhostFrames - 1)
                                 * (float) (table.numFrames - 1);

            buildFramePath (ghostPaths[k], table, ghostPos, area);
        }
    }

    ghostTableIndex = lastTableIndex;
    ghostGeneration = table.generation;
    ghostNumFrames = table.numFrames;
    ghostArea = area;
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

    const float basePosition = juce::jlimit (0.0f, 1.0f,
                                             processor.apvts.getRawParameterValue (posParamId)->load());
    float shown = basePosition;
    const bool live = processor.getLivePosition (oscIndex, shown);
    shown = juce::jlimit (0.0f, 1.0f, shown);

    auto content = bounds.reduced (5.0f);
    auto cursorArea = content.removeFromBottom (10.0f);
    content.removeFromBottom (2.0f);

    if (tableIndex != ghostTableIndex || table.generation != ghostGeneration
        || table.numFrames != ghostNumFrames || content != ghostArea)
    {
        lastTableIndex = tableIndex;
        rebuildGhosts (table, content);
    }

    g.setColour (AstralLookAndFeel::cyan.withAlpha (0.12f));

    for (const auto& ghost : ghostPaths)
        if (! ghost.isEmpty())
            g.strokePath (ghost, juce::PathStrokeType (1.0f));

    juce::Path wave;
    buildFramePath (wave, table, shown * (float) (table.numFrames - 1), content);

    g.setColour (AstralLookAndFeel::cyan.withAlpha (0.22f));
    g.strokePath (wave, juce::PathStrokeType (3.0f));
    g.setColour (AstralLookAndFeel::cyan);
    g.strokePath (wave, juce::PathStrokeType (1.3f));

    // cursor track
    auto track = cursorArea.withSizeKeepingCentre (cursorArea.getWidth(), 3.0f);
    g.setColour (AstralLookAndFeel::deepPurple.brighter (0.35f));
    g.fillRoundedRectangle (track, 1.5f);

    const auto markerAt = [&] (float pos01)
    {
        return juce::Rectangle<float> (3.0f, cursorArea.getHeight())
                   .withCentre ({ track.getX() + pos01 * track.getWidth(), cursorArea.getCentreY() });
    };

    // dim tick for the knob position, so you can see how far modulation moved
    if (live && std::abs (shown - basePosition) > 0.002f)
    {
        g.setColour (AstralLookAndFeel::magenta.withAlpha (0.35f));
        g.fillRoundedRectangle (markerAt (basePosition), 1.5f);
    }

    const auto marker = markerAt (shown);
    g.setColour (AstralLookAndFeel::magenta.withAlpha (live ? 0.55f : 0.45f));
    g.fillRoundedRectangle (marker.expanded (2.0f, 1.0f), 2.0f);
    g.setColour (AstralLookAndFeel::magenta);
    g.fillRoundedRectangle (marker, 1.5f);

    g.setColour (AstralLookAndFeel::textDim.withAlpha (live ? 0.95f : 0.7f));
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.drawText (juce::String (shown * (float) (table.numFrames - 1) + 1.0f, 1)
                    + " / " + juce::String (table.numFrames),
                bounds.reduced (7.0f, 4.0f), juce::Justification::topRight);
}
