cmake_minimum_required(VERSION 3.22)

project(AstralVega VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(AstralVega
    COMPANY_NAME "Astral Vega Audio"
    BUNDLE_ID com.astralvega.astralvega
    PLUGIN_MANUFACTURER_CODE Asvg
    PLUGIN_CODE Avga
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    PRODUCT_NAME "Astral Vega")

juce_generate_juce_header(AstralVega)

target_sources(AstralVega PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SynthVoice.cpp
    Source/Wavetable.cpp
    Source/FXChain.cpp)

target_compile_definitions(AstralVega PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(AstralVega
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
