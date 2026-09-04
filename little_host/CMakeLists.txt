cmake_minimum_required(VERSION 3.22)
project(LittleHost VERSION 1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../audio_effect_framework/cmake")
include(AudioEffectFramework)

aef_setup_dependencies()

set(AEF_BUILD_STANDALONE ON CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/../audio_effect_framework"
                 "${CMAKE_BINARY_DIR}/audio_effect_framework")

juce_add_plugin(LittleHost
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE LtlH
    FORMATS Standalone
    PRODUCT_NAME "Little Host"
    NEEDS_MIDI_INPUT TRUE
)

juce_generate_juce_header(LittleHost)

target_sources(LittleHost PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_include_directories(LittleHost PRIVATE
    "${CMAKE_SOURCE_DIR}/../audio_effect_framework/Source")

aef_apply_plugin_target(LittleHost)

set(LITTLE_HOST_PLUGIN_DIR "${CMAKE_BINARY_DIR}/plugins" CACHE PATH
    "Directory scanned for .kbplug bundles" FORCE)
target_compile_definitions(LittleHost PRIVATE
    LITTLE_HOST_PLUGIN_DIR="${LITTLE_HOST_PLUGIN_DIR}")
add_dependencies(LittleHost_Standalone dynamic_gain dynamic_odr1 dynamic_sansamp_classic)

add_custom_command(TARGET LittleHost_Standalone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/plugins"
            "$<TARGET_FILE_DIR:LittleHost_Standalone>/plugins"
    COMMENT "Deploying .kbplug bundles next to Little Host standalone")
