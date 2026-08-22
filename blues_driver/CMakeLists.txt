cmake_minimum_required(VERSION 3.22)
project(BluesDriver VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../audio_effect_framework/cmake")
include(AudioEffectFramework)

aef_setup_dependencies()

set(AEF_BUILD_STANDALONE ON CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/../audio_effect_framework"
                 "${CMAKE_BINARY_DIR}/audio_effect_framework")

juce_add_plugin(BluesDriver
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE BDrv
    FORMATS Standalone
    PRODUCT_NAME "Blues Driver"
    NEEDS_MIDI_INPUT TRUE
)

juce_generate_juce_header(BluesDriver)

target_sources(BluesDriver PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
)

target_include_directories(BluesDriver PRIVATE
    "${CMAKE_SOURCE_DIR}/../audio_effect_framework/Source")

aef_apply_plugin_target(BluesDriver)
