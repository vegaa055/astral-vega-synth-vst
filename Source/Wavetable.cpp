#include "Wavetable.h"

Wavetable::Wavetable (juce::String nameIn, int numFramesIn, const Recipe& recipe)
    : name (std::move (nameIn)), numFrames (numFramesIn)
{
    juce::dsp::FFT fft (fftOrder);

    std::vector<float> spectrum ((size_t) 2 * frameSize, 0.0f);
    std::vector<float> work ((size_t) 2 * frameSize);

    allocateMips();

    for (int f = 0; f < numFrames; ++f)
    {
        std::fill (spectrum.begin(), spectrum.end(), 0.0f);

        for (int n = 1; n <= maxHarmonic; ++n)
        {
            const auto c = recipe (f, n);
            spectrum[(size_t) 2 * n]     = c.real();
            spectrum[(size_t) 2 * n + 1] = c.imag();
        }

        buildFrameFromSpectrum (fft, spectrum.data(), f, work);
    }

    normaliseByPeak();
}

Wavetable::Wavetable (juce::String nameIn, const float* frameSamples, int numFramesIn)
    : name (std::move (nameIn)), numFrames (numFramesIn)
{
    juce::dsp::FFT fft (fftOrder);

    std::vector<float> spectrum ((size_t) 2 * frameSize);
    std::vector<float> work ((size_t) 2 * frameSize);

    allocateMips();

    for (int f = 0; f < numFrames; ++f)
    {
        std::fill (spectrum.begin(), spectrum.end(), 0.0f);
        std::copy (frameSamples + (size_t) f * frameSize,
                   frameSamples + (size_t) (f + 1) * frameSize,
                   spectrum.begin());

        fft.performRealOnlyForwardTransform (spectrum.data(), true);

        // strip DC offset and the (unusable) Nyquist bin
        spectrum[0] = spectrum[1] = 0.0f;
        spectrum[(size_t) frameSize] = spectrum[(size_t) frameSize + 1] = 0.0f;

        buildFrameFromSpectrum (fft, spectrum.data(), f, work);
    }

    normaliseByPeak();
}

void Wavetable::allocateMips()
{
    mips.resize (numMips);

    for (auto& m : mips)
        m.resize ((size_t) numFrames * (frameSize + 1));
}

void Wavetable::buildFrameFromSpectrum (juce::dsp::FFT& fft, const float* spectrum,
                                        int frameIndex, std::vector<float>& work)
{
    for (int mip = 0; mip < numMips; ++mip)
    {
        std::copy (spectrum, spectrum + (size_t) 2 * frameSize, work.begin());

        for (int n = topHarmonicForMip (mip) + 1; n <= frameSize / 2; ++n)
            work[(size_t) 2 * n] = work[(size_t) 2 * n + 1] = 0.0f;

        fft.performRealOnlyInverseTransform (work.data());

        auto* dest = mips[(size_t) mip].data() + (size_t) frameIndex * (frameSize + 1);
        std::copy (work.begin(), work.begin() + frameSize, dest);
        dest[frameSize] = dest[0];
    }
}

// Normalise the whole table by the brightest full-bandwidth frame, so the
// FFT engine's scaling convention cancels out and morphing keeps its
// relative frame levels.
void Wavetable::normaliseByPeak()
{
    float peak = 0.0f;

    for (int f = 0; f < numFrames; ++f)
    {
        const auto* d = frameData (0, f);
        for (int i = 0; i < frameSize; ++i)
            peak = juce::jmax (peak, std::abs (d[i]));
    }

    if (peak > 0.0f)
        for (auto& m : mips)
            for (auto& s : m)
                s /= peak;
}

int Wavetable::mipForPhaseInc (float phaseIncrement) const noexcept
{
    if (phaseIncrement <= 0.0f)
        return 0;

    const int allowedHarmonics = (int) (0.5f / phaseIncrement);

    for (int mip = 0; mip < numMips; ++mip)
        if (topHarmonicForMip (mip) <= allowedHarmonics)
            return mip;

    return numMips - 1;
}

std::vector<Wavetable> createFactoryWavetables()
{
    std::vector<Wavetable> tables;
    tables.reserve (3);

    // Classic analogue shapes as morph targets: sine -> triangle -> saw -> square.
    // Coefficients are the standard Fourier series for each shape (sine phase).
    tables.emplace_back ("Basic", 4, [] (int frame, int n) -> std::complex<float>
    {
        switch (frame)
        {
            case 0:
                return { 0.0f, n == 1 ? -1.0f : 0.0f };

            case 1:
                if (n % 2 == 1)
                {
                    const float sign = ((n - 1) / 2) % 2 == 0 ? 1.0f : -1.0f;
                    return { 0.0f, -sign / (float) (n * n) };
                }
                return {};

            case 2:
            {
                const float sign = n % 2 == 1 ? 1.0f : -1.0f;
                return { 0.0f, -sign / (float) n };
            }

            case 3:
                if (n % 2 == 1)
                    return { 0.0f, -1.0f / (float) n };
                return {};
        }

        return {};
    });

    // Pulse-width sweep: duty cycle 50% (square) down to 5% (thin, nasal).
    // Fourier series of a centred bipolar pulse: cosine phase, sin(pi*n*d)/n.
    tables.emplace_back ("PWM", 33, [] (int frame, int n) -> std::complex<float>
    {
        const float duty = 0.5f - 0.45f * (float) frame / 32.0f;
        return { std::sin (juce::MathConstants<float>::pi * (float) n * duty) / (float) n, 0.0f };
    });

    // Spectral tilt morph: 1/n^e with e sweeping 2.0 (dark) to 0.7 (screaming).
    tables.emplace_back ("Spectra", 32, [] (int frame, int n) -> std::complex<float>
    {
        const float t = (float) frame / 31.0f;
        const float exponent = 2.0f - 1.3f * t;
        return { 0.0f, -1.0f / std::pow ((float) n, exponent) };
    });

    return tables;
}
