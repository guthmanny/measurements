cmake_minimum_required(VERSION 3.22)
project(DynaComp VERSION 1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../audio_effect_framework/cmake")
include(AudioEffectFramework)

# Precomputed NuDSP NL tables for white_box Dyna Comp (backbone 96 kHz, balanced ROM).
set(NUDSP_NL_TABLE_PROFILE "balanced" CACHE STRING "NuDSP NL LUT bake: none|full|balanced|tiny" FORCE)
set_property(CACHE NUDSP_NL_TABLE_PROFILE PROPERTY STRINGS none full balanced tiny)
set(NUDSP_NL_TABLE_RATES "96000" CACHE STRING "NuDSP baked NL table sample rates" FORCE)
set(NUDSP_NL_TABLE_MODULES "dyna_comp" CACHE STRING "NuDSP NL bake module groups" FORCE)

get_filename_component(_gfx_packs_root "${CMAKE_SOURCE_DIR}/../../atomtheme-gfx-packs" ABSOLUTE)
if(EXISTS "${_gfx_packs_root}/packs")
    set(GFX_PACKS_ROOT "${_gfx_packs_root}" CACHE PATH "Path to atomtheme-gfx-packs checkout" FORCE)
endif()
set(KBUSS_BUNDLE_GFX_PACKS "ON" CACHE STRING "Bundle gfx into .kbplug" FORCE)

aef_setup_dependencies()

set(AEF_BUILD_STANDALONE ON CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/../audio_effect_framework"
                 "${CMAKE_BINARY_DIR}/audio_effect_framework")

juce_add_plugin(DynaComp
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE DyCp
    FORMATS Standalone
    PRODUCT_NAME "Dyna Comp"
    NEEDS_MIDI_INPUT TRUE
)

juce_generate_juce_header(DynaComp)

target_sources(DynaComp PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
)

target_include_directories(DynaComp PRIVATE
    "${CMAKE_SOURCE_DIR}/../audio_effect_framework/Source")

if(NOT MUDSP_ROOT STREQUAL "" AND EXISTS "${MUDSP_ROOT}/CMakeLists.txt")
    target_compile_definitions(DynaComp PRIVATE MUDSP_ROOT="${MUDSP_ROOT}")
endif()

aef_apply_plugin_target(DynaComp)

set(DYNA_COMP_PLUGIN_DIR "${CMAKE_BINARY_DIR}/plugins" CACHE PATH
    "Directory scanned for DynamicDynaComp.kbplug" FORCE)
target_compile_definitions(DynaComp PRIVATE
    DYNA_COMP_PLUGIN_DIR="${DYNA_COMP_PLUGIN_DIR}")

if(TARGET dynamic_dyna_comp)
    add_dependencies(DynaComp_Standalone dynamic_dyna_comp)
endif()

add_custom_command(TARGET DynaComp_Standalone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:DynaComp_Standalone>/plugins"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${DYNA_COMP_PLUGIN_DIR}"
            "$<TARGET_FILE_DIR:DynaComp_Standalone>/plugins"
    COMMENT "Deploying DynamicDynaComp.kbplug next to Dyna Comp standalone")
