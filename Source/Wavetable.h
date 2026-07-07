#pragma once

#include <JuceHeader.h>
#include <complex>

/**
    A morphable wavetable: numFrames single-cycle waveforms of frameSize samples.

    Each frame is stored at several "mipmap" levels, where level k keeps only the
    lowest (1024 >> k) harmonics. At playback time the oscillator picks the level
    whose highest harmonic stays below Nyquist for the note being played, which is
    what keeps high notes alias-free.

    Tables are built from a spectral recipe: a function returning the complex
    amplitude of each harmonic for each frame. Frames are rendered with an inverse
    FFT, so the same code path will later let us import arbitrary/user wavetables
    via a forward FFT.
*/
class Wavetable
{
public:
    static constexpr int frameSize = 2048;
    static constexpr int fftOrder = 11;              // 2^11 == frameSize
    static constexpr int maxHarmonic = frameSize / 2 - 1;
    static constexpr int numMips = 11;               // top harmonic 1023, 512, ... 1

    /** Complex amplitude for harmonic n (1..maxHarmonic) of the given frame. */
    using Recipe = std::function<std::complex<float> (int frameIndex, int harmonic)>;

    Wavetable (juce::String nameIn, int numFramesIn, const Recipe& recipe);

    /** Builds a table from raw single-cycle frames (numFramesIn * frameSize
        samples, e.g. a Serum-format .wav): forward FFT per frame, then the
        same band-limited mipmap chain as the recipe constructor. */
    Wavetable (juce::String nameIn, const float* frameSamples, int numFramesIn);

    juce::String name;
    int numFrames;

    /** Smallest (brightest) mip level that won't alias at this phase increment
        (in cycles per sample). */
    int mipForPhaseInc (float phaseIncrement) const noexcept;

    /** Pointer to one frame's samples; has a guard sample at index frameSize
        (== sample 0) so linear interpolation never needs to wrap. */
    forcedinline const float* frameData (int mip, int frame) const noexcept
    {
        return mips[(size_t) mip].data() + (size_t) frame * (frameSize + 1);
    }

    static int topHarmonicForMip (int mip) noexcept
    {
        return juce::jmax (1, juce::jmin (maxHarmonic, (frameSize / 2) >> mip));
    }

private:
    void allocateMips();
    void buildFrameFromSpectrum (juce::dsp::FFT&, const float* spectrum,
                                 int frameIndex, std::vector<float>& work);
    void normaliseByPeak();

    std::vector<std::vector<float>> mips;
};

/** Builds the factory bank: Basic (sine/tri/saw/square), PWM (pulse-width sweep),
    Spectra (dark-to-bright harmonic tilt). */
std::vector<Wavetable> createFactoryWavetables();

/**
    A single playback head over a Wavetable: phase accumulator with linear
    interpolation across both the waveform and the frame (morph) axis.
    One SynthVoice owns several of these for unison.
*/
class WavetableOscillator
{
public:
    void setTable (const Wavetable* t) noexcept
    {
        table = t;
        updateMip();
    }

    void setFrequency (float hz, double sampleRate) noexcept
    {
        phaseInc = juce::jmin (0.5f, (float) (hz / sampleRate));
        updateMip();
    }

    void setPhase (float newPhase01) noexcept { phase = newPhase01; }

    /** framePos is in frame units: 0 .. numFrames-1 (fractional = crossfade). */
    forcedinline float getNextSample (float framePos) noexcept
    {
        const int f0 = (int) framePos;
        const float frameFrac = framePos - (float) f0;
        const int f1 = juce::jmin (f0 + 1, table->numFrames - 1);

        const float idx = phase * (float) Wavetable::frameSize;
        const int i0 = (int) idx;
        const float sampleFrac = idx - (float) i0;

        const float* a = table->frameData (mip, f0);
        const float* b = table->frameData (mip, f1);

        const float s0 = a[i0] + sampleFrac * (a[i0 + 1] - a[i0]);
        const float s1 = b[i0] + sampleFrac * (b[i0 + 1] - b[i0]);

        phase += phaseInc;
        if (phase >= 1.0f)
            phase -= 1.0f;

        return s0 + frameFrac * (s1 - s0);
    }

private:
    void updateMip() noexcept
    {
        if (table != nullptr)
            mip = table->mipForPhaseInc (phaseInc);
    }

    const Wavetable* table = nullptr;
    float phase = 0.0f, phaseInc = 0.0f;
    int mip = 0;
};
