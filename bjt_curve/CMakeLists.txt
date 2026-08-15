cmake_minimum_required(VERSION 3.22)
project(BjtCurveTracer VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

set(_juce_candidates
    "D:/source/JUCE"
    "$ENV{USERPROFILE}/source/JUCE"
    "${CMAKE_SOURCE_DIR}/../../JUCE"
)
if(NOT WIN32)
    list(APPEND _juce_candidates "/home/guthman/source/JUCE")
    list(PREPEND CMAKE_PREFIX_PATH "/usr/local")
    find_package(JUCE CONFIG QUIET)
endif()

if(JUCE_FOUND)
    message(STATUS "Using installed JUCE: ${JUCE_DIR}")
else()
    set(JUCE_PATH "" CACHE PATH "Path to JUCE source")
    foreach(_candidate IN LISTS _juce_candidates)
        if(_candidate AND EXISTS "${_candidate}/CMakeLists.txt")
            set(JUCE_PATH "${_candidate}" CACHE PATH "Path to JUCE source" FORCE)
            break()
        endif()
    endforeach()
    if(JUCE_PATH STREQUAL "" OR NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
        message(FATAL_ERROR "JUCE not found locally. Expected checkout under D:/source/JUCE or set JUCE_PATH.")
    endif()
    message(STATUS "JUCE found at ${JUCE_PATH}")
    add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
endif()

# Prefer explicit Windows checkouts. Do not use "$ENV{HOME}/..." when HOME is
# empty: on Windows that collapses to absolute root paths like "/NuDSP" and can
# silently pick a stale D:\NuDSP clone over D:\myCode\MuDSP.
set(_atomtheme_candidates
    "D:/myCode/AtomTheme"
    "${CMAKE_SOURCE_DIR}/../../AtomTheme"
    "$ENV{USERPROFILE}/source/AtomTheme"
)
if(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
    list(APPEND _atomtheme_candidates "$ENV{HOME}/source/AtomTheme")
endif()
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
    message(FATAL_ERROR "AtomTheme not found locally. Expected checkout under D:/myCode/AtomTheme or ${CMAKE_SOURCE_DIR}/../../AtomTheme")
endif()

set(_nudsp_candidates
    "D:/myCode/MuDSP"
    "${CMAKE_SOURCE_DIR}/../../MuDSP"
    "$ENV{USERPROFILE}/source/MuDSP"
)
if(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
    list(APPEND _nudsp_candidates "$ENV{HOME}/source/MuDSP")
endif()
set(NUDSP_LOCAL "")
foreach(_candidate IN LISTS _nudsp_candidates)
    if(_candidate AND EXISTS "${_candidate}/CMakeLists.txt")
        set(NUDSP_LOCAL "${_candidate}")
        break()
    endif()
endforeach()

if(NOT NUDSP_LOCAL STREQUAL "")
    message(STATUS "MuDSP found at ${NUDSP_LOCAL}")
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${NUDSP_LOCAL}" "${CMAKE_BINARY_DIR}/nudsp" EXCLUDE_FROM_ALL)
    set(NUDSP_CORE_INCLUDE "${NUDSP_LOCAL}/core")
else()
    message(FATAL_ERROR "MuDSP not found locally. Expected checkout under D:/myCode/MuDSP or ${CMAKE_SOURCE_DIR}/../../MuDSP")
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
