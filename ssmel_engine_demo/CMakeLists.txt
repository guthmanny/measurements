cmake_minimum_required(VERSION 3.22)
project(SsmelEngineDemo VERSION 1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

if(WIN32)
    set(JUCE_PATH "${CMAKE_SOURCE_DIR}/../../JUCE" CACHE PATH "Path to JUCE source")
    if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
        set(JUCE_PATH "D:/source/JUCE")
    endif()
    add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    set(JUCE_PATH "$ENV{HOME}/source/JUCE" CACHE PATH "Path to JUCE source")
    if(EXISTS "${JUCE_PATH}/CMakeLists.txt")
        message(STATUS "Using local JUCE: ${JUCE_PATH}")
        add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
    else()
        list(PREPEND CMAKE_PREFIX_PATH "/usr/local")
        find_package(JUCE CONFIG QUIET)
        if(JUCE_FOUND)
            message(STATUS "Using installed JUCE: ${JUCE_DIR}")
        else()
            message(FATAL_ERROR "JUCE not found. Set JUCE_PATH to a valid JUCE source directory.")
        endif()
    endif()
endif()

include(FetchContent)

set(ATOM_THEME_LOCAL "${CMAKE_CURRENT_SOURCE_DIR}/../../AtomTheme")
get_filename_component(ATOM_THEME_LOCAL "${ATOM_THEME_LOCAL}" ABSOLUTE)
if(NOT EXISTS "${ATOM_THEME_LOCAL}/CMakeLists.txt")
    set(ATOM_THEME_LOCAL "$ENV{HOME}/source/AtomTheme")
    get_filename_component(ATOM_THEME_LOCAL "${ATOM_THEME_LOCAL}" ABSOLUTE)
endif()
if(NOT EXISTS "${ATOM_THEME_LOCAL}/CMakeLists.txt" AND EXISTS "D:/myCode/AtomTheme/CMakeLists.txt")
    set(ATOM_THEME_LOCAL "D:/myCode/AtomTheme")
endif()
if(EXISTS "${ATOM_THEME_LOCAL}/CMakeLists.txt")
    message(STATUS "AtomTheme found at ${ATOM_THEME_LOCAL} — using local checkout")
    set(ATOMTHEME_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${ATOM_THEME_LOCAL}" "${CMAKE_BINARY_DIR}/AtomTheme" EXCLUDE_FROM_ALL)
    set(ATOM_COLLECTIONS_APP_DIR "${ATOM_THEME_LOCAL}/demos/collections_app")
else()
    message(STATUS "AtomTheme not found locally — fetching from GitHub")
    set(ATOMTHEME_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        AtomTheme
        GIT_REPOSITORY https://github.com/guthmanny/AtomTheme.git
        GIT_TAG main
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(AtomTheme)
    set(ATOM_COLLECTIONS_APP_DIR "${AtomTheme_SOURCE_DIR}/demos/collections_app")
endif()

# Closed SSMEL SDK only. Do not FetchContent / add_subdirectory MuDSP for customers.
set(SSMEL_DIR "" CACHE PATH "Directory containing SSMELConfig.cmake (…/lib/cmake/SSMEL)")
set(MUDSP_ROOT "" CACHE PATH "Dev-only in-tree MuDSP (do not ship this path)")

find_package(SSMEL CONFIG QUIET)

if(NOT SSMEL_FOUND AND NOT MUDSP_ROOT STREQUAL "" AND EXISTS "${MUDSP_ROOT}/CMakeLists.txt")
    message(STATUS "SSMEL package not found — using in-tree MuDSP at ${MUDSP_ROOT} (dev only)")
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_SSMEL_ENGINE ON CACHE BOOL "" FORCE)
    set(NUDSP_BUILD_DUMP_NL_TABLES OFF CACHE BOOL "" FORCE)
    add_subdirectory("${MUDSP_ROOT}" "${CMAKE_BINARY_DIR}/MuDSP" EXCLUDE_FROM_ALL)
endif()

if(NOT TARGET SSMEL::ssmel)
    message(FATAL_ERROR
        "SSMEL SDK not found.\n"
        "  Release: cmake -DSSMEL_DIR=/path/to/ssmel-sdk/lib/cmake/SSMEL\n"
        "  Dev:     cmake -DMUDSP_ROOT=/path/to/MuDSP")
endif()

juce_add_plugin(SsmelEngineDemo
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE Ssme
    FORMATS Standalone
    PRODUCT_NAME "SsmelEngineDemo"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
)

juce_generate_juce_header(SsmelEngineDemo)

target_sources(SsmelEngineDemo PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/SsmelDemoMap.cpp
    Source/SsmelDemoMap.h
    Source/AppSettingsPanel.cpp
    Source/AppSettingsPanel.h
    Source/StandaloneMain.cpp
    ${ATOM_COLLECTIONS_APP_DIR}/AudioSettingsPanel.cpp
    ${ATOM_COLLECTIONS_APP_DIR}/AudioSettingsPanel.h
)

target_include_directories(SsmelEngineDemo PRIVATE
    "${ATOM_COLLECTIONS_APP_DIR}"
    "${CMAKE_SOURCE_DIR}/Source"
)

target_compile_definitions(SsmelEngineDemo
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        $<$<NOT:$<PLATFORM_ID:Windows>>:JUCE_JACK=1>
    PRIVATE
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
        SSMEL_USE_SHARED
        SSMEL_ENGINE_USE_SHARED
        SSMEL_DEMO_SOUND_DIR="${CMAKE_SOURCE_DIR}/example_sound_sources/sources"
)

if(WIN32)
    target_compile_definitions(SsmelEngineDemo PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

target_link_libraries(SsmelEngineDemo PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_dsp
    juce::juce_atom_theme
    SSMEL::ssmel
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
