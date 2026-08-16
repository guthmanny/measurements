cmake_minimum_required(VERSION 3.22)
project(Ds1OpampAcTracer VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

# Parent of measurements/ (e.g. D:/myCode) holds sibling checkouts.
get_filename_component(_DEPS_ROOT "${CMAKE_SOURCE_DIR}/../.." ABSOLUTE)

# ---------------------------------------------------------------------------
# JUCE
# ---------------------------------------------------------------------------
set(_juce_candidates
    "${_DEPS_ROOT}/JUCE"
    "D:/source/JUCE"
)
set(JUCE_PATH "" CACHE PATH "Path to JUCE source")
foreach(_candidate IN LISTS _juce_candidates)
    if(_candidate AND EXISTS "${_candidate}/CMakeLists.txt")
        set(JUCE_PATH "${_candidate}" CACHE PATH "Path to JUCE source" FORCE)
        break()
    endif()
endforeach()
if(JUCE_PATH STREQUAL "" OR NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found. Tried: ${_juce_candidates}")
endif()
message(STATUS "JUCE found at ${JUCE_PATH}")
add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")

# ---------------------------------------------------------------------------
# AtomTheme (UI)
# ---------------------------------------------------------------------------
set(ATOM_THEME_LOCAL "${_DEPS_ROOT}/AtomTheme")
if(NOT EXISTS "${ATOM_THEME_LOCAL}/CMakeLists.txt")
    message(FATAL_ERROR "AtomTheme not found at ${ATOM_THEME_LOCAL}")
endif()
message(STATUS "AtomTheme found at ${ATOM_THEME_LOCAL}")
set(ATOMTHEME_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
add_subdirectory("${ATOM_THEME_LOCAL}" "${CMAKE_BINARY_DIR}/AtomTheme" EXCLUDE_FROM_ALL)

# ---------------------------------------------------------------------------
# MuDSP + kBuss (parent-relative siblings)
# kBuss pulls MuDSP via MUDSP_ROOT; do not add MuDSP twice.
# ---------------------------------------------------------------------------
set(MUDSP_ROOT "${_DEPS_ROOT}/MuDSP" CACHE PATH "Path to local MuDSP checkout" FORCE)
if(NOT EXISTS "${MUDSP_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "MuDSP not found at ${MUDSP_ROOT}")
endif()

set(KBUSS_ROOT "${_DEPS_ROOT}/kbuss" CACHE PATH "Path to local kBuss checkout" FORCE)
if(NOT EXISTS "${KBUSS_ROOT}/CMakeLists.txt")
    # Windows may use mixed case; accept kBuss as alternate folder name.
    set(KBUSS_ROOT "${_DEPS_ROOT}/kBuss" CACHE PATH "Path to local kBuss checkout" FORCE)
endif()
if(NOT EXISTS "${KBUSS_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "kBuss not found at ${_DEPS_ROOT}/kbuss (or kBuss)")
endif()

message(STATUS "MuDSP root: ${MUDSP_ROOT}")
message(STATUS "kBuss found at ${KBUSS_ROOT}")
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(NUDSP_BUILD_DUMP_NL_TABLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${KBUSS_ROOT}" "${CMAKE_BINARY_DIR}/kbuss" EXCLUDE_FROM_ALL)

if(TARGET nudsp AND NOT TARGET NuDSP::nudsp)
    add_library(NuDSP::nudsp ALIAS nudsp)
endif()
if(TARGET MuDSP::mudsp AND NOT TARGET NuDSP::nudsp)
    add_library(NuDSP::nudsp ALIAS MuDSP::mudsp)
endif()

# ---------------------------------------------------------------------------
# App
# ---------------------------------------------------------------------------
juce_add_gui_app(Ds1OpampAcTracer
    PRODUCT_NAME "DS-1 OpAmp AC Tracer"
    VERSION 0.1.0
    COMPANY_NAME "nux"
)

juce_generate_juce_header(Ds1OpampAcTracer)

target_sources(Ds1OpampAcTracer PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    Source/Ds1OpampAcMath.cpp
    Source/Ds1OpampAcMath.h
    Source/Ds1OpampAcPanel.cpp
    Source/Ds1OpampAcPanel.h
)

target_compile_definitions(Ds1OpampAcTracer PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="DS-1 OpAmp AC Tracer"
    JUCE_APPLICATION_VERSION_STRING="0.1.0"
)

if(WIN32)
    target_compile_definitions(Ds1OpampAcTracer PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(UNIX AND NOT APPLE)
    target_compile_definitions(Ds1OpampAcTracer PRIVATE
        JUCE_ALSA=1
        JUCE_PULSEAUDIO=1
        JUCE_JACK=1
    )
endif()

target_link_libraries(Ds1OpampAcTracer PRIVATE
    juce::juce_atom_theme
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    NuDSP::nudsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

target_include_directories(Ds1OpampAcTracer PRIVATE "${MUDSP_ROOT}/core")
