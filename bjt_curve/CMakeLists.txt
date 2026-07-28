cmake_minimum_required(VERSION 3.22)
project(BjtCurveTracer VERSION 0.1.0 LANGUAGES CXX)
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
if(EXISTS "${ATOM_THEME_LOCAL}/CMakeLists.txt")
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
    juce_add_module("${AtomTheme_SOURCE_DIR}/modules/juce_atom_theme" ALIAS_NAMESPACE juce)
endif()

set(_nudsp_candidates
    "$ENV{HOME}/source/MuDSP"
    "$ENV{HOME}/NuDSP"
    "${CMAKE_SOURCE_DIR}/../../MuDSP"
)
set(NUDSP_LOCAL "")
foreach(_candidate IN LISTS _nudsp_candidates)
    if(EXISTS "${_candidate}/CMakeLists.txt")
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

juce_add_gui_app(BjtCurveTracer
    PRODUCT_NAME "BJT Curve Tracer"
    VERSION 0.1.0
    COMPANY_NAME "nux"
)

juce_generate_juce_header(BjtCurveTracer)

target_sources(BjtCurveTracer PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    Source/BjtCurveMath.cpp
    Source/BjtCurveMath.h
    Source/BjtCurvePanel.cpp
    Source/BjtCurvePanel.h
)

target_compile_definitions(BjtCurveTracer PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="BJT Curve Tracer"
    JUCE_APPLICATION_VERSION_STRING="0.1.0"
)

if(UNIX AND NOT APPLE)
    target_compile_definitions(BjtCurveTracer PRIVATE
        JUCE_ALSA=1
        JUCE_PULSEAUDIO=1
        JUCE_JACK=1
    )
endif()

target_link_libraries(BjtCurveTracer PRIVATE
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
    target_include_directories(BjtCurveTracer PRIVATE "${NUDSP_CORE_INCLUDE}")
endif()

add_executable(bjt_curve_diag tools/bjt_curve_diag.cpp Source/BjtCurveMath.cpp)
target_include_directories(bjt_curve_diag PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
target_compile_definitions(bjt_curve_diag PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 JUCE_USE_CURL=0)
target_link_libraries(bjt_curve_diag PRIVATE juce::juce_core NuDSP::nudsp)
if(DEFINED NUDSP_CORE_INCLUDE)
    target_include_directories(bjt_curve_diag PRIVATE "${NUDSP_CORE_INCLUDE}")
endif()
