cmake_minimum_required(VERSION 3.22)
project(FreqMapMixGuide VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

if(APPLE)
    set(FM_FORMATS VST3 AU Standalone)
else()
    set(FM_FORMATS VST3 Standalone)
endif()

option(FM_COPY_AFTER_BUILD "Kopiuj wtyczke do systemowego folderu VST3 po buildzie" ON)

juce_add_plugin(FreqMapMixGuide
    COMPANY_NAME "BrainScorcherAudio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${FM_COPY_AFTER_BUILD}
    PLUGIN_MANUFACTURER_CODE Fmmg
    PLUGIN_CODE Fmmx
    FORMATS ${FM_FORMATS}
    VST3_CATEGORIES Fx Analyzer
    AU_MAIN_TYPE kAudioUnitType_Effect
    PRODUCT_NAME "FreqMap MixGuide")

target_sources(FreqMapMixGuide PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SpectrumAnalyzer.cpp
    Source/GuideData.cpp)

target_compile_definitions(FreqMapMixGuide PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_compile_options(FreqMapMixGuide PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_link_libraries(FreqMapMixGuide
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
