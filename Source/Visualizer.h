#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/** Oscilloscope / spectrum analyser fed from the processor's lock-free scope
    FIFO. Click to toggle between the two modes. */
class Visualizer : public juce::Component,
                   private juce::Timer
{
public:
    explicit Visualizer (AstralVegaAudioProcessor&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void drawScope (juce::Graphics&, juce::Rectangle<float>);
    void drawSpectrum (juce::Graphics&, juce::Rectangle<float>);

    static constexpr int ringSize = 4096;      // power of two
    static constexpr int scopeLength = 1024;
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;

    AstralVegaAudioProcessor& processor;

    std::array<float, ringSize> ring {};
    int ringPos = 0;

    juce::dsp::FFT fft { fftOrder };
    std::array<float, (size_t) fftSize * 2> fftData {};
    std::array<float, (size_t) fftSize> window {};
    std::array<float, (size_t) fftSize / 2> smoothedMagnitudes {};

    bool spectrumMode = false;
};
