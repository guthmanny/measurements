cmake_minimum_required(VERSION 3.22)
project(CamelDemo VERSION 1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

# AtomTheme requires JUCE 8+ APIs; prefer a local source tree over /usr/local JUCE 7.
set(CMAKE_PREFIX_PATH "")
if(EXISTS "$ENV{HOME}/source/JUCE/CMakeLists.txt")
    set(JUCE_PATH "$ENV{HOME}/source/JUCE" CACHE PATH "JUCE source tree" FORCE)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../JUCE/CMakeLists.txt")
    set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../JUCE" CACHE PATH "JUCE source tree" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../audio_effect_framework/cmake")
include(AudioEffectFramework)

aef_setup_dependencies()

set(AEF_BUILD_STANDALONE ON CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/../audio_effect_framework"
                 "${CMAKE_BINARY_DIR}/audio_effect_framework")

juce_add_plugin(CamelDemo
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE CmDm
    FORMATS Standalone
    PRODUCT_NAME "CAMEL Demo"
    NEEDS_MIDI_INPUT TRUE
)

juce_generate_juce_header(CamelDemo)

target_sources(CamelDemo PRIVATE
    Source/CamelEffectEngine.cpp
    Source/CamelEffectEngine.h
    Source/CamelCatalog.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_include_directories(CamelDemo PRIVATE
    "${CMAKE_SOURCE_DIR}/../audio_effect_framework/Source"
    "${CMAKE_SOURCE_DIR}/Source")

aef_apply_plugin_target(CamelDemo)
