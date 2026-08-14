cmake_minimum_required(VERSION 3.22)
project(Ds1 VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../audio_effect_framework/cmake")
include(AudioEffectFramework)

# Precomputed NuDSP NL tables for white_box DS-1 (compile-time; no runtime switch).
# Use none to fall back to Newton tabulate on prepare.
set(NUDSP_NL_TABLE_PROFILE "full" CACHE STRING "NuDSP NL LUT bake: none|full|balanced|tiny")
set_property(CACHE NUDSP_NL_TABLE_PROFILE PROPERTY STRINGS none full balanced tiny)
set(NUDSP_NL_TABLE_RATES "48000" CACHE STRING "NuDSP baked NL table sample rates")
set(NUDSP_NL_TABLE_MODULES "ds1" CACHE STRING "NuDSP NL bake module groups")

aef_setup_dependencies()

set(AEF_BUILD_STANDALONE ON CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/../audio_effect_framework"
                 "${CMAKE_BINARY_DIR}/audio_effect_framework")

juce_add_plugin(Ds1
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE Ds1x
    FORMATS Standalone
    PRODUCT_NAME "DS-1"
    NEEDS_MIDI_INPUT TRUE
)

juce_generate_juce_header(Ds1)

target_sources(Ds1 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
)

target_include_directories(Ds1 PRIVATE
    "${CMAKE_SOURCE_DIR}/../audio_effect_framework/Source")

aef_apply_plugin_target(Ds1)
