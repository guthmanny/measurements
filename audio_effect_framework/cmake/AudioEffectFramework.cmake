# Resolves and adds JUCE, AtomTheme, and kbuss. Sets:
#   ATOM_COLLECTIONS_APP_DIR, KBUSS_DIR, AEF_ASIO_SDK_INCLUDE (Windows)

get_filename_component(AEF_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(AEF_SOURCE_DIR "${AEF_ROOT_DIR}" CACHE INTERNAL "")
set(AEF_EDITOR_CPP "${AEF_ROOT_DIR}/Source/AudioEffectFrameworkEditor.cpp" CACHE INTERNAL "")

# MuDSP — forwarded to kbuss before add_subdirectory.
set(MUDSP_ROOT "" CACHE PATH "Path to local MuDSP checkout (skips fetch when valid)")
set(KBUSS_ROOT "" CACHE PATH "Path to local kbuss checkout (skips auto-detect when valid)")

macro(aef_alias_mudsp)
    if(TARGET nudsp AND NOT TARGET MuDSP::mudsp)
        add_library(MuDSP::mudsp ALIAS nudsp)
    elseif(TARGET NuDSP::nudsp AND NOT TARGET MuDSP::mudsp)
        add_library(MuDSP::mudsp ALIAS NuDSP::nudsp)
    endif()
endmacro()

function(aef_resolve_mudsp_root)
    if(NOT MUDSP_ROOT STREQUAL "" AND EXISTS "${MUDSP_ROOT}/CMakeLists.txt")
        return()
    endif()

    get_filename_component(_aef_mudsp_root "${CMAKE_SOURCE_DIR}" ABSOLUTE)
    foreach(_candidate
        "${_aef_mudsp_root}/../../MuDSP"
        "D:/myCode/MuDSP"
        "$ENV{HOME}/MuDSP"
        "$ENV{USERPROFILE}/MuDSP")
        if(EXISTS "${_candidate}/CMakeLists.txt")
            set(MUDSP_ROOT "${_candidate}" CACHE PATH "Path to local MuDSP checkout" FORCE)
            break()
        endif()
    endforeach()
endfunction()
function(aef_setup_dependencies)
    get_filename_component(_aef_deps_root "${CMAKE_SOURCE_DIR}" ABSOLUTE)

    aef_resolve_mudsp_root()

    if(WIN32)
        set(ASIO_SDK_PATH "" CACHE PATH "Path to Steinberg ASIO SDK root")

        if(ASIO_SDK_PATH STREQUAL "" AND DEFINED ENV{ASIO_SDK_PATH})
            set(ASIO_SDK_PATH "$ENV{ASIO_SDK_PATH}" CACHE PATH "Path to Steinberg ASIO SDK root" FORCE)
        endif()

        if(ASIO_SDK_PATH STREQUAL "" AND EXISTS "C:/asiosdk/common/iasiodrv.h")
            set(ASIO_SDK_PATH "C:/asiosdk" CACHE PATH "Path to Steinberg ASIO SDK root" FORCE)
        endif()

        if(ASIO_SDK_PATH AND EXISTS "${ASIO_SDK_PATH}/common/iasiodrv.h")
            set(AEF_ASIO_SDK_INCLUDE "${ASIO_SDK_PATH}/common" PARENT_SCOPE)
            message(STATUS "ASIO SDK: ${ASIO_SDK_PATH}")
        else()
            message(WARNING
                "JUCE_ASIO will be enabled but ASIO SDK was not found. "
                "Set ASIO_SDK_PATH (or env ASIO_SDK_PATH) to the SDK root.")
        endif()
    endif()

    if(WIN32)
        set(JUCE_PATH "${CMAKE_SOURCE_DIR}/../JUCE" CACHE PATH "Path to JUCE source")
        get_filename_component(JUCE_PATH "${JUCE_PATH}" ABSOLUTE)
        if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
            set(JUCE_PATH "${_aef_deps_root}/../../JUCE")
            get_filename_component(JUCE_PATH "${JUCE_PATH}" ABSOLUTE)
        endif()
        if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
            set(JUCE_PATH "$ENV{HOME}/source/JUCE")
            get_filename_component(JUCE_PATH "${JUCE_PATH}" ABSOLUTE)
        endif()
        if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
            message(FATAL_ERROR
                "JUCE not found. Set JUCE_PATH to a JUCE 7+ source tree with CMake support.")
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
                    "JUCE not found under /usr/local and no source tree at ${JUCE_PATH}.")
            endif()
            message(STATUS "Using JUCE source tree: ${JUCE_PATH}")
            add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
        endif()
    endif()

    include(FetchContent)

    get_filename_component(ATOM_THEME_LOCAL "${_aef_deps_root}/../../AtomTheme" ABSOLUTE)
    if(NOT EXISTS "${ATOM_THEME_LOCAL}/CMakeLists.txt")
        set(ATOM_THEME_LOCAL "$ENV{HOME}/myCode/AtomTheme")
        get_filename_component(ATOM_THEME_LOCAL "${ATOM_THEME_LOCAL}" ABSOLUTE)
    endif()
    if(NOT EXISTS "${ATOM_THEME_LOCAL}/CMakeLists.txt")
        set(ATOM_THEME_LOCAL "$ENV{HOME}/source/AtomTheme")
        get_filename_component(ATOM_THEME_LOCAL "${ATOM_THEME_LOCAL}" ABSOLUTE)
    endif()

    if(EXISTS "${ATOM_THEME_LOCAL}/CMakeLists.txt")
        message(STATUS "AtomTheme found at ${ATOM_THEME_LOCAL} — using local checkout")
        set(ATOMTHEME_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
        add_subdirectory("${ATOM_THEME_LOCAL}"
                         "${CMAKE_BINARY_DIR}/AtomTheme"
                         EXCLUDE_FROM_ALL)
        set(_atom_dir "${ATOM_THEME_LOCAL}/demos/collections_app")
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
        set(_atom_dir "${AtomTheme_SOURCE_DIR}/demos/collections_app")
    endif()
    set(ATOM_COLLECTIONS_APP_DIR "${_atom_dir}" PARENT_SCOPE)

    get_filename_component(KBUSS_LOCAL "${_aef_deps_root}/../../kbuss" ABSOLUTE)
    if(NOT KBUSS_ROOT STREQUAL "" AND EXISTS "${KBUSS_ROOT}/CMakeLists.txt")
        set(KBUSS_LOCAL "${KBUSS_ROOT}")
    elseif(NOT EXISTS "${KBUSS_LOCAL}/CMakeLists.txt")
        set(KBUSS_LOCAL "${_aef_deps_root}/../../kBuss")
        get_filename_component(KBUSS_LOCAL "${KBUSS_LOCAL}" ABSOLUTE)
    endif()
    if(NOT EXISTS "${KBUSS_LOCAL}/CMakeLists.txt")
        set(KBUSS_LOCAL "$ENV{HOME}/myCode/kbuss")
        get_filename_component(KBUSS_LOCAL "${KBUSS_LOCAL}" ABSOLUTE)
    endif()
    if(NOT EXISTS "${KBUSS_LOCAL}/CMakeLists.txt")
        set(KBUSS_LOCAL "$ENV{HOME}/source/kbuss")
        get_filename_component(KBUSS_LOCAL "${KBUSS_LOCAL}" ABSOLUTE)
    endif()
    if(NOT EXISTS "${KBUSS_LOCAL}/CMakeLists.txt")
        set(KBUSS_LOCAL "D:/myCode/kbuss")
        get_filename_component(KBUSS_LOCAL "${KBUSS_LOCAL}" ABSOLUTE)
    endif()
    if(EXISTS "${KBUSS_LOCAL}/CMakeLists.txt")
        message(STATUS "kbuss found at ${KBUSS_LOCAL} — using local checkout")
        set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
        add_subdirectory("${KBUSS_LOCAL}"
                         "${CMAKE_BINARY_DIR}/kbuss"
                         EXCLUDE_FROM_ALL)
        set(_kbuss_dir "${KBUSS_LOCAL}")
    else()
        message(FATAL_ERROR
            "kbuss not found. Expected a sibling checkout at ../../kbuss (or set -DKBUSS_ROOT=<path>).")
    endif()
    aef_alias_mudsp()
    set(KBUSS_DIR "${_kbuss_dir}" PARENT_SCOPE)
endfunction()

# Apply common compile/link settings to a juce_add_plugin target.
function(aef_apply_plugin_target target)
    target_sources(${target} PRIVATE
        "${AEF_EDITOR_CPP}"
        "${AEF_SOURCE_DIR}/Source/AefStandaloneMain.cpp")

    target_include_directories(${target} PRIVATE "${AEF_SOURCE_DIR}/Source")

    target_compile_features(${target} PRIVATE cxx_std_20)

    target_compile_definitions(${target}
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            $<$<NOT:$<PLATFORM_ID:Windows>>:JUCE_JACK=1>
        PRIVATE
            JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
    )

    if(WIN32)
        target_compile_definitions(${target} PRIVATE
            NOMINMAX
            WIN32_LEAN_AND_MEAN
            JUCE_ASIO=1
        )
        if(AEF_ASIO_SDK_INCLUDE)
            target_include_directories(${target} PRIVATE "${AEF_ASIO_SDK_INCLUDE}")
        endif()
    endif()

    if(NOT MSVC)
        target_compile_options(${target} PRIVATE
            -Wno-switch-enum
            -Wno-overloaded-virtual
        )
    endif()

    target_link_libraries(${target} PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_dsp
        juce::juce_atom_theme
        audio_effect_framework
        kbuss_plugin
        MuDSP::mudsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
    )
endfunction()
