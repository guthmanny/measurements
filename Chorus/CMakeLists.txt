cmake_minimum_required(VERSION 3.22)
project(Chorus VERSION 1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../audio_effect_framework/cmake")
include(AudioEffectFramework)

aef_setup_dependencies()

set(AEF_BUILD_STANDALONE ON CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/../audio_effect_framework"
                 "${CMAKE_BINARY_DIR}/audio_effect_framework")

juce_add_plugin(Chorus
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE Chrs
    FORMATS Standalone
    PRODUCT_NAME "Chorus"
    NEEDS_MIDI_INPUT TRUE
)

juce_generate_juce_header(Chorus)

target_sources(Chorus PRIVATE
    Source/ChorusEffectEngine.cpp
    Source/ChorusEffectEngine.h
    Source/ChorusNuDspDefaults.cpp
    Source/ChorusNuDspDefaults.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_include_directories(Chorus PRIVATE
    "${CMAKE_SOURCE_DIR}/../audio_effect_framework/Source"
    "${CMAKE_SOURCE_DIR}/Source")

aef_apply_plugin_target(Chorus)
