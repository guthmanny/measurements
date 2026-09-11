cmake_minimum_required(VERSION 3.22)
project(LittleHost VERSION 1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../audio_effect_framework/cmake")
include(AudioEffectFramework)

set(NUDSP_NL_TABLE_PROFILE "balanced" CACHE STRING "NuDSP NL LUT bake: none|full|balanced|tiny" FORCE)
set_property(CACHE NUDSP_NL_TABLE_PROFILE PROPERTY STRINGS none full balanced tiny)
set(NUDSP_NL_TABLE_RATES "96000" CACHE STRING "NuDSP baked NL table sample rates" FORCE)
set(NUDSP_NL_TABLE_MODULES "ds1" CACHE STRING "NuDSP NL bake module groups" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_SSMEL_ENGINE OFF CACHE BOOL "" FORCE)

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

set(_little_host_white_box_lab "${CMAKE_SOURCE_DIR}/../white_box_lab")

target_sources(LittleHost PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    ../common/SchematicSignalOutput.cpp
    ../common/SchematicSignalOutput.h
    ${_little_host_white_box_lab}/Source/CircuitBindingResolver.cpp
    ${_little_host_white_box_lab}/Source/CircuitBindingResolver.h
    ${_little_host_white_box_lab}/Source/ComponentValueCodec.cpp
    ${_little_host_white_box_lab}/Source/ComponentValueCodec.h
    ${_little_host_white_box_lab}/Source/MudspAssetResolver.cpp
    ${_little_host_white_box_lab}/Source/MudspAssetResolver.h
    ${_little_host_white_box_lab}/Source/SchematicEditorPanel.cpp
    ${_little_host_white_box_lab}/Source/SchematicEditorPanel.h
    ${_little_host_white_box_lab}/Source/SvgCircuitParser.cpp
    ${_little_host_white_box_lab}/Source/SvgCircuitParser.h
    ${_little_host_white_box_lab}/Source/SvgCircuitSync.cpp
    ${_little_host_white_box_lab}/Source/SvgCircuitSync.h
)

target_include_directories(LittleHost PRIVATE
    "${CMAKE_SOURCE_DIR}/../audio_effect_framework/Source"
    "${CMAKE_SOURCE_DIR}/../common"
    "${_little_host_white_box_lab}/Source")

if(NOT MUDSP_ROOT STREQUAL "" AND EXISTS "${MUDSP_ROOT}/CMakeLists.txt")
    target_compile_definitions(LittleHost PRIVATE MUDSP_ROOT="${MUDSP_ROOT}")
endif()

aef_apply_plugin_target(LittleHost)

set(LITTLE_HOST_PLUGIN_DIR "${CMAKE_BINARY_DIR}/plugins" CACHE PATH
    "Directory scanned for .kbplug bundles" FORCE)
target_compile_definitions(LittleHost PRIVATE
    LITTLE_HOST_PLUGIN_DIR="${LITTLE_HOST_PLUGIN_DIR}")
add_dependencies(LittleHost_Standalone dynamic_ds1)

add_custom_command(TARGET LittleHost_Standalone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:LittleHost_Standalone>/plugins"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/plugins"
            "$<TARGET_FILE_DIR:LittleHost_Standalone>/plugins"
    COMMENT "Deploying .kbplug bundles next to Little Host standalone")
