cmake_minimum_required(VERSION 3.22)
project(Ds1OpampAcTracer VERSION 0.1.0 LANGUAGES CXX)
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

set(_atomtheme_candidates
    "$ENV{HOME}/source/AtomTheme"
    "$ENV{USERPROFILE}/source/AtomTheme"
    "D:/myCode/AtomTheme"
    "${CMAKE_SOURCE_DIR}/../../AtomTheme"
)
set(ATOM_THEME_LOCAL "")
foreach(_candidate IN LISTS _atomtheme_candidates)
    if(_candidate AND EXISTS "${_candidate}/CMakeLists.txt")
        set(ATOM_THEME_LOCAL "${_candidate}")
        break()
    endif()
endforeach()

if(NOT ATOM_THEME_LOCAL STREQUAL "")
    message(STATUS "AtomTheme found at ${ATOM_THEME_LOCAL}")
    juce_add_module("${ATOM_THEME_LOCAL}/modules/juce_atom_theme" ALIAS_NAMESPACE juce)
else()
    FetchContent_Declare(
        AtomTheme
        GIT_REPOSITORY https://github.com/guthmanny/AtomTheme.git
        GIT_TAG main
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(AtomTheme)
endif()

set(_nudsp_candidates
    "$ENV{HOME}/source/MuDSP"
    "$ENV{USERPROFILE}/source/MuDSP"
    "$ENV{HOME}/NuDSP"
    "$ENV{USERPROFILE}/NuDSP"
    "D:/myCode/NuDSP"
    "${CMAKE_SOURCE_DIR}/../../MuDSP"
    "${CMAKE_SOURCE_DIR}/../../NuDSP"
)
set(NUDSP_LOCAL "")
foreach(_candidate IN LISTS _nudsp_candidates)
    if(_candidate AND EXISTS "${_candidate}/CMakeLists.txt")
        set(NUDSP_LOCAL "${_candidate}")
        break()
    endif()
endforeach()

if(NOT NUDSP_LOCAL STREQUAL "")
    message(STATUS "NuDSP found at ${NUDSP_LOCAL}")
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${NUDSP_LOCAL}" "${CMAKE_BINARY_DIR}/nudsp" EXCLUDE_FROM_ALL)
    set(NUDSP_CORE_INCLUDE "${NUDSP_LOCAL}/core")
else()
    message(STATUS "NuDSP not found locally — fetching from GitHub")
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        NuDSP
        GIT_REPOSITORY https://github.com/guthmanny/NuDSP.git
        GIT_TAG develop
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(NuDSP)
    set(NUDSP_CORE_INCLUDE "${NuDSP_SOURCE_DIR}/core")
endif()

if(TARGET nudsp AND NOT TARGET NuDSP::nudsp)
    add_library(NuDSP::nudsp ALIAS nudsp)
endif()

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

if(DEFINED NUDSP_CORE_INCLUDE)
    target_include_directories(Ds1OpampAcTracer PRIVATE "${NUDSP_CORE_INCLUDE}")
endif()
