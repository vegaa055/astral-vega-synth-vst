#pragma once

#include <JuceHeader.h>

/**
    Preset handling: a factory bank defined in code (as overrides on top of
    parameter defaults, so new parameters never break old presets) plus user
    presets stored as APVTS XML in Documents/Astral Vega/Presets.

    Indices run factory first, then user files sorted by name.
*/
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState&);

    juce::StringArray getPresetNames();          // rescans the user folder
    int getCurrentIndex() const { return currentIndex; }
    juce::String getCurrentName() const { return currentName; }

    void loadPreset (int index);
    void loadNext();
    void loadPrevious();

    juce::File getUserPresetDirectory() const;
    void saveUserPreset (juce::File file);

    /** Called after a user preset's full state has been restored via
        replaceState — lets the processor react (e.g. reload the user
        wavetable referenced by the state). */
    std::function<void()> onStateLoaded;

private:
    struct FactoryPreset
    {
        const char* name;
        std::vector<std::pair<const char*, float>> overrides;   // paramID -> plain value
    };

    static const std::vector<FactoryPreset>& getFactoryPresets();

    void resetAllToDefaults();
    void setPlain (const char* paramID, float plainValue);
    void refreshUserFiles();
    int totalPresets() const;

    juce::AudioProcessorValueTreeState& apvts;
    juce::Array<juce::File> userFiles;
    int currentIndex = 0;
    juce::String currentName { "Init" };
};
