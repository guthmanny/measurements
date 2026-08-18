cmake_minimum_required(VERSION 3.22)
project(Chorus VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

# Steinberg ASIO SDK (Windows only). Download from Steinberg developer site and set
# ASIO_SDK_PATH to the SDK root (the folder that contains common/iasiodrv.h).
if(WIN32)
    set(ASIO_SDK_PATH "" CACHE PATH "Path to Steinberg ASIO SDK root")

    if(ASIO_SDK_PATH STREQUAL "" AND DEFINED ENV{ASIO_SDK_PATH})
        set(ASIO_SDK_PATH "$ENV{ASIO_SDK_PATH}" CACHE PATH "Path to Steinberg ASIO SDK root" FORCE)
    endif()

    if(ASIO_SDK_PATH STREQUAL "" AND EXISTS "C:/asiosdk/common/iasiodrv.h")
        set(ASIO_SDK_PATH "C:/asiosdk" CACHE PATH "Path to Steinberg ASIO SDK root" FORCE)
    endif()

    if(ASIO_SDK_PATH AND EXISTS "${ASIO_SDK_PATH}/common/iasiodrv.h")
        set(CHORUS_ASIO_SDK_INCLUDE "${ASIO_SDK_PATH}/common")
        message(STATUS "ASIO SDK: ${ASIO_SDK_PATH}")
    else()
        message(WARNING
            "JUCE_ASIO will be enabled but ASIO SDK was not found. "
            "Set ASIO_SDK_PATH (or env ASIO_SDK_PATH) to the SDK root, e.g. "
            "-DASIO_SDK_PATH=\"C:/asiosdk\"")
    endif()
endif()

# JUCE discovery: Windows builds from source; Linux prefers /usr/local install.
if(WIN32)
    set(JUCE_PATH "${CMAKE_SOURCE_DIR}/../JUCE" CACHE PATH "Path to JUCE source")
    if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
        if(EXISTS "D:/source/JUCE/CMakeLists.txt")
            set(JUCE_PATH "D:/source/JUCE")
        else()
            message(FATAL_ERROR
                "JUCE not found. Set JUCE_PATH to a JUCE 7+ source tree with CMake support.")
        endif()
    endif()
    add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    list(PREPEND CMAKE_PREFIX_PATH "/usr/local")
    find_package(JUCE CONFIG QUIET)

    if(JUCE_FOUND)
        message(STATUS "Using installed JUCE: ${JUCE_DIR}")
    else()
        set(JUCE_PATH "${CMAKE_SOURCE_DIR}/../JUCE" CACHE PATH "Path to JUCE source")
        if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
            message(FATAL_ERROR
                "JUCE not found under /usr/local and no source tree at ${JUCE_PATH}. "
                "Install JUCE to /usr/local or set JUCE_PATH.")
        endif()
        message(STATUS "Using JUCE source tree: ${JUCE_PATH}")
        add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
    endif()
endif()

# ---------------------------------------------------------------------------
# AtomTheme (https://github.com/guthmanny/AtomTheme) — JUCE UI theme module
#
# Resolution order:
#   1. $ENV{HOME}/source/AtomTheme   — local development checkout
#   2. Common Windows checkout paths
#   3. FetchContent from GitHub      — automatic download for third-party builds
#
# AtomTheme's CMakeLists already calls juce_add_module, so after
# add_subdirectory / FetchContent_MakeAvailable we only need the demos path.
# ---------------------------------------------------------------------------
include(FetchContent)

set(ATOM_THEME_LOCAL "$ENV{HOME}/source/AtomTheme")
if(NOT EXISTS "${ATOM_THEME_LOCAL}/CMakeLists.txt" AND EXISTS "D:/myCode/AtomTheme/CMakeLists.txt")
    set(ATOM_THEME_LOCAL "D:/myCode/AtomTheme")
elseif(NOT EXISTS "${ATOM_THEME_LOCAL}/CMakeLists.txt" AND EXISTS "D:/source/AtomTheme/CMakeLists.txt")
    set(ATOM_THEME_LOCAL "D:/source/AtomTheme")
endif()

if(EXISTS "${ATOM_THEME_LOCAL}/CMakeLists.txt")
    message(STATUS "AtomTheme found at ${ATOM_THEME_LOCAL} — using local checkout")
    set(ATOMTHEME_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${ATOM_THEME_LOCAL}"
                     "${CMAKE_BINARY_DIR}/AtomTheme"
                     EXCLUDE_FROM_ALL)
    set(ATOM_COLLECTIONS_APP_DIR "${ATOM_THEME_LOCAL}/demos/collections_app")
else()
    message(STATUS "AtomTheme not found locally — fetching from GitHub")
    set(ATOMTHEME_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        AtomTheme
        GIT_REPOSITORY  https://github.com/guthmanny/AtomTheme.git
        GIT_TAG         main
        GIT_SHALLOW     TRUE
    )
    FetchContent_MakeAvailable(AtomTheme)
    set(ATOM_COLLECTIONS_APP_DIR "${AtomTheme_SOURCE_DIR}/demos/collections_app")
endif()

# ---------------------------------------------------------------------------
# kbuss — audio plugin host engine (bundles MuDSP)
# ---------------------------------------------------------------------------
set(MUDSP_ROOT "" CACHE PATH "Path to local MuDSP checkout")
set(KBUSS_ROOT "" CACHE PATH "Path to local kbuss checkout")

get_filename_component(_chorus_deps_root "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(KBUSS_LOCAL "${_chorus_deps_root}/../kbuss")
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
    add_subdirectory("${KBUSS_LOCAL}"
                     "${CMAKE_BINARY_DIR}/kbuss"
                     EXCLUDE_FROM_ALL)
    set(KBUSS_DIR "${KBUSS_LOCAL}")
else()
    message(FATAL_ERROR
        "kbuss not found. Expected a sibling checkout at ../kbuss (or set -DKBUSS_ROOT=<path>).")
endif()
if(TARGET nudsp AND NOT TARGET MuDSP::mudsp)
    add_library(MuDSP::mudsp ALIAS nudsp)
endif()

juce_add_plugin(Chorus
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE Chrs
    FORMATS Standalone
    PRODUCT_NAME "Chorus"
    NEEDS_MIDI_INPUT TRUE
)

juce_generate_juce_header(Chorus)


target_sources(Chorus PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/ChorusHeaderComponent.cpp
    Source/ChorusHeaderComponent.h
    Source/ChorusFooterComponent.cpp
    Source/ChorusFooterComponent.h
    Source/TunerOverlayComponent.cpp
    Source/TunerOverlayComponent.h
    Source/SpectrumOverlayComponent.cpp
    Source/SpectrumOverlayComponent.h
    Source/SpectrumCurveComponent.cpp
    Source/SpectrumCurveComponent.h
    Source/SpectrumAnalyzer.h
    Source/TunerDetector.h
    Source/KbussChorusEngine.cpp
    Source/KbussChorusEngine.h
    Source/MinibussChorusEngine.h
    Source/AppSettingsPanel.cpp
    Source/AppSettingsPanel.h
    Source/NoiseGateSettingsPanel.cpp
    Source/NoiseGateSettingsPanel.h
    Source/PeakDisplaySettingsPanel.cpp
    Source/PeakDisplaySettingsPanel.h
    Source/CalibrationSettingsPanel.cpp
    Source/CalibrationSettingsPanel.h
    Source/ChorusMidiCcSettingsPanel.cpp
    Source/ChorusMidiCcSettingsPanel.h
    Source/ChorusSettingsPanel.cpp
    Source/ChorusSettingsPanel.h
    Source/ChorusNuDspDefaults.cpp
    Source/ChorusNuDspDefaults.h
    Source/MeterDisplayUtils.h
    Source/CompressorCurveComponent.cpp
    Source/CompressorCurveComponent.h
    Source/ChorusLfoCurveComponent.cpp
    Source/ChorusLfoCurveComponent.h
    Source/CompressorKnee.h
    Source/PluginParameter.h
    Source/JackCaptureRouting.cpp
    Source/JackCaptureRouting.h
    Source/StandaloneMain.cpp
    ${ATOM_COLLECTIONS_APP_DIR}/AudioSettingsPanel.cpp
    ${ATOM_COLLECTIONS_APP_DIR}/AudioSettingsPanel.h
)

target_include_directories(Chorus PRIVATE
    "${ATOM_COLLECTIONS_APP_DIR}"
    "${KBUSS_DIR}/include"
    "${KBUSS_DIR}/src"
)

if(TARGET MuDSP::mudsp)
    get_target_property(_mudsp_inc MuDSP::mudsp INTERFACE_INCLUDE_DIRECTORIES)
    if(_mudsp_inc)
        target_include_directories(Chorus PRIVATE ${_mudsp_inc})
    endif()
endif()

target_compile_features(Chorus PRIVATE cxx_std_20)

target_compile_definitions(Chorus
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        $<$<NOT:$<PLATFORM_ID:Windows>>:JUCE_JACK=1>
    PRIVATE
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
)

if(WIN32)
    # windows.h min/max macros break std::min/std::max and JUCE audio headers.
    target_compile_definitions(Chorus PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        JUCE_ASIO=1
    )

    if(CHORUS_ASIO_SDK_INCLUDE)
        target_include_directories(Chorus PRIVATE "${CHORUS_ASIO_SDK_INCLUDE}")
    endif()
endif()

if(NOT MSVC)
    target_compile_options(Chorus PRIVATE
        -Wno-switch-enum
        -Wno-overloaded-virtual
    )
endif()

target_link_libraries(Chorus PRIVATE
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
