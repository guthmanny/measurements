cmake_minimum_required(VERSION 3.22)
project(WhiteBoxLab VERSION 1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_PREFIX_PATH "")
if(EXISTS "$ENV{HOME}/source/JUCE/CMakeLists.txt")
    set(JUCE_PATH "$ENV{HOME}/source/JUCE" CACHE PATH "JUCE source tree" FORCE)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../JUCE/CMakeLists.txt")
    set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../JUCE" CACHE PATH "JUCE source tree" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../audio_effect_framework/cmake")
include(AudioEffectFramework)

set(NUDSP_NL_TABLE_PROFILE "none" CACHE STRING "NuDSP NL LUT bake: none|full|balanced|tiny")
set_property(CACHE NUDSP_NL_TABLE_PROFILE PROPERTY STRINGS none full balanced tiny)
set(NUDSP_NL_TABLE_RATES "48000" CACHE STRING "NuDSP baked NL table sample rates")
set(NUDSP_NL_TABLE_MODULES "ds1" CACHE STRING "NuDSP NL bake module groups")

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_SSMEL_ENGINE OFF CACHE BOOL "" FORCE)

aef_setup_dependencies()

set(AEF_BUILD_STANDALONE ON CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/../audio_effect_framework"
                 "${CMAKE_BINARY_DIR}/audio_effect_framework")

juce_add_plugin(WhiteBoxLab
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE WbLb
    FORMATS Standalone
    PRODUCT_NAME "White Box Lab"
    NEEDS_MIDI_INPUT TRUE
)

juce_generate_juce_header(WhiteBoxLab)

target_sources(WhiteBoxLab PRIVATE
    ../common/SchematicSignalOutput.cpp
    ../common/SchematicSignalOutput.h
    Source/AllParamsPanel.cpp
    Source/AllParamsPanel.h
    Source/CircuitBindingResolver.cpp
    Source/CircuitBindingResolver.h
    Source/ComponentValueCodec.cpp
    Source/ComponentValueCodec.h
    Source/MudspAssetResolver.cpp
    Source/MudspAssetResolver.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/SchematicEditorPanel.cpp
    Source/SchematicEditorPanel.h
    Source/SvgCircuitParser.cpp
    Source/SvgCircuitParser.h
    Source/SvgCircuitSync.cpp
    Source/SvgCircuitSync.h
    Source/WhiteBoxEffectEngine.cpp
    Source/WhiteBoxEffectEngine.h
)

target_include_directories(WhiteBoxLab PRIVATE
    "${CMAKE_SOURCE_DIR}/../common"
    "${CMAKE_SOURCE_DIR}/../audio_effect_framework/Source"
    "${CMAKE_SOURCE_DIR}/Source")

if(NOT MUDSP_ROOT STREQUAL "" AND EXISTS "${MUDSP_ROOT}/CMakeLists.txt")
    target_compile_definitions(WhiteBoxLab PRIVATE MUDSP_ROOT="${MUDSP_ROOT}")
endif()

aef_apply_plugin_target(WhiteBoxLab)
