#include "Visualizer.h"
#include "AstralLookAndFeel.h"

Visualizer::Visualizer (AstralVegaAudioProcessor& p)
    : processor (p)
{
    for (size_t i = 0; i < (size_t) fftSize; ++i)
        window[i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi
                                            * (float) i / (float) fftSize);

    setOpaque (false);
    startTimerHz (30);
}

void Visualizer::mouseDown (const juce::MouseEvent&)
{
    spectrumMode = ! spectrumMode;
    repaint();
}

void Visualizer::timerCallback()
{
    float temp[512];
    int numRead = 0;

    while ((numRead = processor.readScopeSamples (temp, 512)) > 0)
        for (int i = 0; i < numRead; ++i)
        {
            ring[(size_t) ringPos] = temp[i];
            ringPos = (ringPos + 1) & (ringSize - 1);
        }

    repaint();
}

void Visualizer::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (AstralLookAndFeel::background.brighter (0.03f));
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (AstralLookAndFeel::panelOutline);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    auto plot = bounds.reduced (10.0f, 8.0f);

    // centre line / baseline grid
    g.setColour (AstralLookAndFeel::panelOutline.withAlpha (0.15f));
    g.drawHorizontalLine ((int) plot.getCentreY(), plot.getX(), plot.getRight());

    if (spectrumMode)
        drawSpectrum (g, plot);
    else
        drawScope (g, plot);

    g.setColour (AstralLookAndFeel::textDim.withAlpha (0.6f));
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawText (spectrumMode ? "SPECTRUM" : "SCOPE",
                bounds.reduced (10.0f, 5.0f), juce::Justification::topRight);
}

void Visualizer::drawScope (juce::Graphics& g, juce::Rectangle<float> plot)
{
    // find a rising zero crossing to (roughly) stabilise pitched waveforms
    int start = (ringPos - scopeLength * 2 + ringSize * 2) & (ringSize - 1);
    int triggered = (ringPos - scopeLength + ringSize) & (ringSize - 1);

    for (int i = 1; i < scopeLength; ++i)
    {
        const int idx = (start + i) & (ringSize - 1);
        const int prev = (start + i - 1) & (ringSize - 1);

        if (ring[(size_t) prev] < 0.0f && ring[(size_t) idx] >= 0.0f)
        {
            triggered = idx;
            break;
        }
    }

    juce::Path wave;

    for (int i = 0; i < scopeLength; ++i)
    {
        const float sample = juce::jlimit (-1.0f, 1.0f,
                                           ring[(size_t) ((triggered + i) & (ringSize - 1))]);
        const float px = plot.getX() + plot.getWidth() * (float) i / (float) (scopeLength - 1);
        const float py = plot.getCentreY() - sample * plot.getHeight() * 0.48f;

        if (i == 0)
            wave.startNewSubPath (px, py);
        else
            wave.lineTo (px, py);
    }

    g.setColour (AstralLookAndFeel::cyan.withAlpha (0.25f));
    g.strokePath (wave, juce::PathStrokeType (3.5f));
    g.setColour (AstralLookAndFeel::cyan);
    g.strokePath (wave, juce::PathStrokeType (1.4f));
}

void Visualizer::drawSpectrum (juce::Graphics& g, juce::Rectangle<float> plot)
{
    const double sampleRate = processor.getSampleRate() > 0.0 ? processor.getSampleRate()
                                                              : 44100.0;

    std::fill (fftData.begin(), fftData.end(), 0.0f);

    const int start = (ringPos - fftSize + ringSize) & (ringSize - 1);

    for (int i = 0; i < fftSize; ++i)
        fftData[(size_t) i] = ring[(size_t) ((start + i) & (ringSize - 1))] * window[(size_t) i];

    fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

    for (int b = 1; b < fftSize / 2; ++b)
    {
        const float magnitude = fftData[(size_t) b] * (4.0f / (float) fftSize);
        const float db = juce::Decibels::gainToDecibels (magnitude, -100.0f);
        const float norm = juce::jlimit (0.0f, 1.0f, juce::jmap (db, -84.0f, 0.0f, 0.0f, 1.0f));

        auto& smoothed = smoothedMagnitudes[(size_t) b];
        smoothed = juce::jmax (norm, smoothed * 0.86f);   // fast attack, slow release
    }

    juce::Path curve;
    bool started = false;
    const float logRange = std::log (1000.0f);   // 20 Hz .. 20 kHz

    for (int b = 1; b < fftSize / 2; ++b)
    {
        const float freq = (float) b * (float) sampleRate / (float) fftSize;

        if (freq < 20.0f)
            continue;

        if (freq > 20000.0f)
            break;

        const float px = plot.getX() + plot.getWidth() * std::log (freq / 20.0f) / logRange;
        const float py = plot.getBottom() - smoothedMagnitudes[(size_t) b] * plot.getHeight();

        if (! started)
        {
            curve.startNewSubPath (px, py);
            started = true;
        }
        else
        {
            curve.lineTo (px, py);
        }
    }

    if (! started)
        return;

    juce::Path fill (curve);
    fill.lineTo (plot.getRight(), plot.getBottom());
    fill.lineTo (plot.getX(), plot.getBottom());
    fill.closeSubPath();

    g.setGradientFill (juce::ColourGradient (AstralLookAndFeel::magenta.withAlpha (0.35f),
                                             plot.getX(), plot.getY(),
                                             AstralLookAndFeel::magenta.withAlpha (0.02f),
                                             plot.getX(), plot.getBottom(), false));
    g.fillPath (fill);

    g.setColour (AstralLookAndFeel::cyan);
    g.strokePath (curve, juce::PathStrokeType (1.4f));
}
