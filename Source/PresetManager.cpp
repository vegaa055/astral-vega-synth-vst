#include "PresetManager.h"

namespace
{
    struct FileNameSorter
    {
        static int compareElements (const juce::File& a, const juce::File& b)
        {
            return a.getFileName().compareNatural (b.getFileName());
        }
    };
}

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state)
    : apvts (state)
{
    refreshUserFiles();
}

juce::File PresetManager::getUserPresetDirectory() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("Astral Vega")
                   .getChildFile ("Presets");
    dir.createDirectory();
    return dir;
}

void PresetManager::refreshUserFiles()
{
    userFiles = getUserPresetDirectory().findChildFiles (juce::File::findFiles, false, "*.xml");

    FileNameSorter sorter;
    userFiles.sort (sorter);
}

juce::StringArray PresetManager::getPresetNames()
{
    refreshUserFiles();

    juce::StringArray names;

    for (const auto& preset : getFactoryPresets())
        names.add (preset.name);

    for (const auto& file : userFiles)
        names.add (file.getFileNameWithoutExtension());

    return names;
}

int PresetManager::totalPresets() const
{
    return (int) getFactoryPresets().size() + userFiles.size();
}

void PresetManager::loadPreset (int index)
{
    const auto& factory = getFactoryPresets();
    const int numFactory = (int) factory.size();

    if (index < 0 || index >= totalPresets())
        return;

    if (index < numFactory)
    {
        const auto& preset = factory[(size_t) index];

        resetAllToDefaults();

        for (const auto& [id, value] : preset.overrides)
            setPlain (id, value);

        currentName = preset.name;
    }
    else
    {
        const auto file = userFiles[index - numFactory];

        if (auto xml = juce::parseXML (file))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            currentName = file.getFileNameWithoutExtension();

            if (onStateLoaded != nullptr)
                onStateLoaded();
        }
    }

    currentIndex = index;
}

void PresetManager::loadNext()
{
    loadPreset ((currentIndex + 1) % totalPresets());
}

void PresetManager::loadPrevious()
{
    loadPreset ((currentIndex - 1 + totalPresets()) % totalPresets());
}

void PresetManager::saveUserPreset (juce::File file)
{
    if (file.getFileExtension() != ".xml")
        file = file.withFileExtension (".xml");

    if (auto xml = apvts.copyState().createXml())
        xml->writeTo (file, {});

    refreshUserFiles();
    currentName = file.getFileNameWithoutExtension();
    currentIndex = (int) getFactoryPresets().size() + userFiles.indexOf (file);
}

void PresetManager::resetAllToDefaults()
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            ranged->setValueNotifyingHost (ranged->getDefaultValue());
}

void PresetManager::setPlain (const char* paramID, float plainValue)
{
    if (auto* p = apvts.getParameter (juce::String (paramID)))
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
    else
        jassertfalse;   // preset refers to a parameter ID that no longer exists
}

const std::vector<PresetManager::FactoryPreset>& PresetManager::getFactoryPresets()
{
    // Values are plain (denormalised); anything not listed stays at its default.
    // Mod sources: 1 LFO1, 2 LFO2, 3 Env2, 4 Velocity, 5 ModWheel.
    // Mod targets: 1 APos, 2 BPos, 3 ALvl, 4 BLvl, 5 Pitch, 6 Cutoff, 7 Res, 8 Morph.
    static const std::vector<FactoryPreset> presets = {

        { "Init", {} },

        { "Supersaw Lead", {
            { "oscAUnison", 7.0f }, { "oscADetune", 28.0f }, { "oscASpread", 1.0f },
            { "oscBLevel", 0.45f }, { "oscBPos", 0.66f }, { "oscBCoarse", -12.0f },
            { "oscBUnison", 5.0f }, { "oscBDetune", 22.0f }, { "oscBSpread", 0.8f },
            { "filterCutoff", 9000.0f }, { "release", 0.35f },
            { "chorusOn", 1.0f },
            { "delayOn", 1.0f }, { "delaySync", 1.0f }, { "delayDiv", 8.0f },   // dotted 1/8
            { "delayFeedback", 0.45f }, { "delayMix", 0.28f },
            { "reverbOn", 1.0f }, { "reverbSize", 0.7f }, { "reverbMix", 0.25f } } },

        { "Retro Pump Pad", {
            { "oscAUnison", 7.0f }, { "oscADetune", 20.0f }, { "oscASpread", 1.0f },
            { "oscBLevel", 0.4f }, { "oscBPos", 0.66f }, { "oscBCoarse", -12.0f },
            { "attack", 0.4f }, { "sustain", 0.9f }, { "release", 1.2f },
            { "filterCutoff", 4000.0f },
            { "chorusOn", 1.0f },
            { "reverbOn", 1.0f }, { "reverbSize", 0.8f }, { "reverbMix", 0.35f },
            { "pumpOn", 1.0f }, { "pumpAmount", 0.6f },
            { "pumpSync", 1.0f }, { "pumpDiv", 6.0f } } },   // 1/4 notes

        { "PWM Strings", {
            { "oscATable", 1.0f }, { "oscAPos", 0.2f },
            { "oscAUnison", 5.0f }, { "oscADetune", 18.0f }, { "oscASpread", 0.9f },
            { "lfo2Rate", 0.4f },
            { "mod1Src", 2.0f }, { "mod1Dst", 1.0f }, { "mod1Amt", 0.35f },
            { "attack", 0.25f }, { "release", 0.8f },
            { "filterCutoff", 8000.0f },
            { "chorusOn", 1.0f },
            { "reverbOn", 1.0f }, { "reverbMix", 0.3f } } },

        { "808 Slide", {
            { "voiceMode", 1.0f }, { "glideTime", 0.09f },
            { "oscALevel", 0.0f },
            { "subLevel", 0.95f },
            { "distOn", 1.0f }, { "distDrive", 0.35f }, { "distMix", 0.8f },
            { "attack", 0.002f }, { "decay", 0.3f }, { "sustain", 0.8f }, { "release", 0.45f },
            { "filterCutoff", 2000.0f },
            { "gain", -3.0f } } },

        { "Acid Pluck", {
            { "voiceMode", 1.0f }, { "glideTime", 0.05f },
            { "filterCutoff", 320.0f }, { "filterRes", 4.0f },
            { "filterDrive", 0.45f }, { "filterEnvAmt", 0.55f },
            { "env2Attack", 0.001f }, { "env2Decay", 0.18f },
            { "env2Sustain", 0.0f }, { "env2Release", 0.1f },
            { "decay", 0.25f }, { "sustain", 0.25f }, { "release", 0.2f } } },

        { "Lo-fi Keys", {
            { "oscAPos", 0.15f },
            { "attack", 0.003f }, { "decay", 0.5f }, { "sustain", 0.6f }, { "release", 0.5f },
            { "filterCutoff", 6500.0f },
            { "mod1Src", 4.0f }, { "mod1Dst", 6.0f }, { "mod1Amt", 0.3f },
            { "crushOn", 1.0f }, { "crushBits", 10.0f }, { "crushRate", 6.0f },
            { "chorusOn", 1.0f }, { "chorusMix", 0.3f },
            { "reverbOn", 1.0f }, { "reverbSize", 0.45f }, { "reverbMix", 0.3f } } },

        { "Wobble Bass", {
            { "voiceMode", 1.0f },
            { "oscAUnison", 3.0f }, { "oscADetune", 12.0f },
            { "oscBLevel", 0.5f }, { "oscBPos", 0.66f }, { "oscBCoarse", -12.0f },
            { "filterCutoff", 900.0f }, { "filterRes", 2.5f },
            { "lfo1Sync", 1.0f }, { "lfo1Div", 9.0f },   // 1/8 wobble on the grid
            { "mod1Src", 1.0f }, { "mod1Dst", 6.0f }, { "mod1Amt", -0.55f },
            { "distOn", 1.0f }, { "distDrive", 0.3f } } },

        { "Neon Vibrato Lead", {
            { "oscATable", 2.0f }, { "oscAPos", 0.5f },
            { "oscAUnison", 3.0f }, { "oscADetune", 14.0f },
            { "voiceMode", 2.0f }, { "glideTime", 0.06f },
            { "lfo2Rate", 5.5f },
            { "mod1Src", 2.0f }, { "mod1Dst", 5.0f }, { "mod1Amt", 0.03f },
            { "filterCutoff", 10000.0f },
            { "delayOn", 1.0f }, { "delayTime", 300.0f }, { "delayFeedback", 0.4f }, { "delayMix", 0.3f },
            { "reverbOn", 1.0f }, { "reverbMix", 0.25f } } },

        { "Dark PWM Bass", {
            { "voiceMode", 1.0f }, { "glideTime", 0.04f },
            { "oscATable", 1.0f }, { "oscAPos", 0.35f },
            { "subLevel", 0.6f },
            { "filterCutoff", 700.0f }, { "filterDrive", 0.5f },
            { "decay", 0.4f }, { "sustain", 0.6f }, { "release", 0.25f },
            { "distOn", 1.0f }, { "distDrive", 0.25f } } },
    };

    return presets;
}
