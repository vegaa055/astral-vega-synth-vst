#pragma once

#include <JuceHeader.h>

/** Shared beat-division table for everything that can tempo-sync
    (LFO rates, delay time, pump rate). Indices are stable — append only. */
namespace TempoDivisions
{
    inline const juce::StringArray& names()
    {
        static const juce::StringArray n { "4 bars", "2 bars", "1 bar", "1/2", "1/2T", "1/4D",
                                           "1/4", "1/4T", "1/8D", "1/8", "1/8T",
                                           "1/16", "1/16T", "1/32" };
        return n;
    }

    inline double beats (int index)
    {
        static const double b[] = { 16.0, 8.0, 4.0, 2.0, 4.0 / 3.0, 1.5,
                                    1.0, 2.0 / 3.0, 0.75, 0.5, 1.0 / 3.0,
                                    0.25, 1.0 / 6.0, 0.125 };
        return b[juce::jlimit (0, 13, index)];
    }

    inline float toHz (int index, double bpm)
    {
        return (float) (bpm / (60.0 * beats (index)));
    }

    inline float toMs (int index, double bpm)
    {
        return (float) (beats (index) * 60000.0 / bpm);
    }

    constexpr int quarterNote = 6;
    constexpr int dottedEighth = 8;
    constexpr int eighthNote = 9;
}
