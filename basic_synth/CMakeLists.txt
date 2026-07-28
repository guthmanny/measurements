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
if(EXISTS "${ATOM_THEME_LOCAL}/CMakeLists.txt")
    juce_add_module("${ATOM_THEME_LOCAL}/modules/juce_atom_theme" ALIAS_NAMESPACE juce)
    set(ATOM_COLLECTIONS_APP_DIR "${ATOM_THEME_LOCAL}/demos/collections_app")
else()
    FetchContent_Declare(
        AtomTheme
        GIT_REPOSITORY https://github.com/guthmanny/AtomTheme.git
        GIT_TAG main
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(AtomTheme)
    juce_add_module("${AtomTheme_SOURCE_DIR}/modules/juce_atom_theme" ALIAS_NAMESPACE juce)
    set(ATOM_COLLECTIONS_APP_DIR "${AtomTheme_SOURCE_DIR}/demos/collections_app")
endif()

set(MINIBUSS_LOCAL "$ENV{HOME}/source/minibuss")
if(EXISTS "${MINIBUSS_LOCAL}/CMakeLists.txt")
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${MINIBUSS_LOCAL}" "${CMAKE_BINARY_DIR}/minibuss" EXCLUDE_FROM_ALL)
else()
    FetchContent_Declare(
        minibuss
        GIT_REPOSITORY https://github.com/guthmanny/minibuss.git
        GIT_TAG main
        GIT_SHALLOW TRUE
    )
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(minibuss)
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

if(EXISTS "${MINIBUSS_LOCAL}/CMakeLists.txt")
    set(MINIBUSS_DIR "${MINIBUSS_LOCAL}")
else()
    set(MINIBUSS_DIR "${minibuss_SOURCE_DIR}")
endif()

target_sources(BasicSynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/AppSettingsPanel.cpp
    Source/AppSettingsPanel.h
    Source/MinibussSynthEngine.cpp
    Source/MinibussSynthEngine.h
    Source/StandaloneMain.cpp
    ${ATOM_COLLECTIONS_APP_DIR}/AudioSettingsPanel.cpp
    ${ATOM_COLLECTIONS_APP_DIR}/AudioSettingsPanel.h
)

target_include_directories(BasicSynth PRIVATE
    "${ATOM_COLLECTIONS_APP_DIR}"
    "${MINIBUSS_DIR}/include"
    "${MINIBUSS_DIR}/src"
)

target_compile_definitions(BasicSynth
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_JACK=1
    PRIVATE
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
)

target_link_libraries(BasicSynth PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_dsp
    juce::juce_atom_theme
    minibuss_plugin
    NuDSP::nudsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

add_executable(basic_synth_verify_sound tests/verify_sound.cpp
    Source/MinibussSynthEngine.cpp
    Source/MinibussSynthEngine.h
)
target_include_directories(basic_synth_verify_sound PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
    "${MINIBUSS_DIR}/include"
    "${MINIBUSS_DIR}/src"
)
target_link_libraries(basic_synth_verify_sound PRIVATE minibuss_plugin NuDSP::nudsp)
add_test(NAME basic_synth_verify_sound COMMAND basic_synth_verify_sound)
