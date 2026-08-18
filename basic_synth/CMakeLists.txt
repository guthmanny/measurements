cmake_minimum_required(VERSION 3.22)
project(BasicSynth VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

if(WIN32)
    set(JUCE_PATH "${CMAKE_SOURCE_DIR}/../../JUCE" CACHE PATH "Path to JUCE source")
    if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
        set(JUCE_PATH "D:/source/JUCE")
    endif()
    add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    list(PREPEND CMAKE_PREFIX_PATH "/usr/local")
    find_package(JUCE CONFIG QUIET)
    if(JUCE_FOUND)
        message(STATUS "Using installed JUCE: ${JUCE_DIR}")
    else()
        set(JUCE_PATH "${CMAKE_SOURCE_DIR}/../../JUCE" CACHE PATH "Path to JUCE source")
        if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
            set(JUCE_PATH "/home/guthman/source/JUCE")
        endif()
        add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
    endif()
endif()

include(FetchContent)

set(ATOM_THEME_LOCAL "$ENV{HOME}/source/AtomTheme")
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

set(MUDSP_ROOT "" CACHE PATH "Path to local MuDSP checkout")
set(KBUSS_ROOT "" CACHE PATH "Path to local kbuss checkout")

get_filename_component(_synth_deps_root "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(KBUSS_LOCAL "${_synth_deps_root}/../kbuss")
get_filename_component(KBUSS_LOCAL "${KBUSS_LOCAL}" ABSOLUTE)
if(NOT KBUSS_ROOT STREQUAL "" AND EXISTS "${KBUSS_ROOT}/CMakeLists.txt")
    set(KBUSS_LOCAL "${KBUSS_ROOT}")
elseif(NOT EXISTS "${KBUSS_LOCAL}/CMakeLists.txt")
    set(KBUSS_LOCAL "D:/myCode/kbuss")
    get_filename_component(KBUSS_LOCAL "${KBUSS_LOCAL}" ABSOLUTE)
endif()
if(NOT EXISTS "${KBUSS_LOCAL}/CMakeLists.txt")
    set(KBUSS_LOCAL "$ENV{HOME}/source/kbuss")
    get_filename_component(KBUSS_LOCAL "${KBUSS_LOCAL}" ABSOLUTE)
endif()
if(EXISTS "${KBUSS_LOCAL}/CMakeLists.txt")
    message(STATUS "kbuss found at ${KBUSS_LOCAL} — using local checkout")
    if(NOT MUDSP_ROOT STREQUAL "" AND EXISTS "${MUDSP_ROOT}/CMakeLists.txt")
        set(MUDSP_DIR "${MUDSP_ROOT}" CACHE PATH "" FORCE)
    endif()
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${KBUSS_LOCAL}" "${CMAKE_BINARY_DIR}/kbuss" EXCLUDE_FROM_ALL)
    set(KBUSS_DIR "${KBUSS_LOCAL}")
else()
    message(FATAL_ERROR
        "kbuss not found. Expected a sibling checkout at ../kbuss (or set -DKBUSS_ROOT=<path>).")
endif()
if(TARGET nudsp AND NOT TARGET MuDSP::mudsp)
    add_library(MuDSP::mudsp ALIAS nudsp)
endif()

juce_add_plugin(BasicSynth
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE Bsyn
    FORMATS Standalone
    PRODUCT_NAME "Basic Synth"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
)

juce_generate_juce_header(BasicSynth)

target_sources(BasicSynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/AppSettingsPanel.cpp
    Source/AppSettingsPanel.h
    Source/KbussSynthEngine.cpp
    Source/KbussSynthEngine.h
    Source/MinibussSynthEngine.h
    Source/StandaloneMain.cpp
    ${ATOM_COLLECTIONS_APP_DIR}/AudioSettingsPanel.cpp
    ${ATOM_COLLECTIONS_APP_DIR}/AudioSettingsPanel.h
)

target_include_directories(BasicSynth PRIVATE
    "${ATOM_COLLECTIONS_APP_DIR}"
    "${KBUSS_DIR}/include"
    "${KBUSS_DIR}/src"
)

target_compile_definitions(BasicSynth
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        $<$<NOT:$<PLATFORM_ID:Windows>>:JUCE_JACK=1>
    PRIVATE
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
)

if(WIN32)
    target_compile_definitions(BasicSynth PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

target_link_libraries(BasicSynth PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_dsp
    juce::juce_atom_theme
    kbuss_plugin
    MuDSP::mudsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

add_executable(basic_synth_verify_sound tests/verify_sound.cpp
    Source/KbussSynthEngine.cpp
    Source/KbussSynthEngine.h
    Source/MinibussSynthEngine.h
)
target_include_directories(basic_synth_verify_sound PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
    "${KBUSS_DIR}/include"
    "${KBUSS_DIR}/src"
)
target_link_libraries(basic_synth_verify_sound PRIVATE kbuss_plugin MuDSP::mudsp)
if(WIN32)
    target_compile_definitions(basic_synth_verify_sound PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
add_test(NAME basic_synth_verify_sound COMMAND basic_synth_verify_sound)
