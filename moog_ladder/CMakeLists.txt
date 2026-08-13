cmake_minimum_required(VERSION 3.22)
project(MoogLadder VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../audio_effect_framework/cmake")
include(AudioEffectFramework)

# No precomputed NL tables needed for Moog Ladder (no white-box NL).
set(NUDSP_NL_TABLE_PROFILE "none" CACHE STRING "NuDSP NL LUT bake: none|full|balanced|tiny")
set_property(CACHE NUDSP_NL_TABLE_PROFILE PROPERTY STRINGS none full balanced tiny)
set(NUDSP_NL_TABLE_RATES "" CACHE STRING "NuDSP baked NL table sample rates")
set(NUDSP_NL_TABLE_MODULES "" CACHE STRING "NuDSP NL bake module groups")

aef_setup_dependencies()

set(AEF_BUILD_STANDALONE ON CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/../audio_effect_framework"
                 "${CMAKE_BINARY_DIR}/audio_effect_framework")

juce_add_plugin(MoogLadder
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE Moog
    FORMATS Standalone
    PRODUCT_NAME "Moog Ladder"
    NEEDS_MIDI_INPUT TRUE
)

juce_generate_juce_header(MoogLadder)

target_sources(MoogLadder PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/MoogLadderEffectEngine.cpp
    Source/MoogLadderEffectEngine.h
    Source/MoogLadderFrCurveComponent.cpp
    Source/MoogLadderFrCurveComponent.h
    Source/MoogLadderEditor.cpp
    Source/MoogLadderEditor.h
    Source/StandaloneMain.cpp
)

target_include_directories(MoogLadder PRIVATE
    "${CMAKE_SOURCE_DIR}/../audio_effect_framework/Source")

aef_apply_plugin_target(MoogLadder)
