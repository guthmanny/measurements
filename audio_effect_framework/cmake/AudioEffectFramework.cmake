# Resolves and adds JUCE, AtomTheme, and minibuss. Sets:
#   ATOM_COLLECTIONS_APP_DIR, MINIBUSS_DIR, AEF_ASIO_SDK_INCLUDE (Windows)

get_filename_component(AEF_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(AEF_SOURCE_DIR "${AEF_ROOT_DIR}" CACHE INTERNAL "")
set(AEF_EDITOR_CPP "${AEF_ROOT_DIR}/Source/AudioEffectFrameworkEditor.cpp" CACHE INTERNAL "")

function(aef_setup_dependencies)
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
                    "JUCE not found under /usr/local and no source tree at ${JUCE_PATH}.")
            endif()
            message(STATUS "Using JUCE source tree: ${JUCE_PATH}")
            add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
        endif()
    endif()

    include(FetchContent)

    if(WIN32)
        if(EXISTS "D:/myCode/AtomTheme/CMakeLists.txt")
            set(ATOM_THEME_LOCAL "D:/myCode/AtomTheme")
        elseif(EXISTS "D:/source/AtomTheme/CMakeLists.txt")
            set(ATOM_THEME_LOCAL "D:/source/AtomTheme")
        endif()
    endif()

    if(NOT ATOM_THEME_LOCAL OR NOT EXISTS "${ATOM_THEME_LOCAL}/CMakeLists.txt")
        set(ATOM_THEME_LOCAL "$ENV{HOME}/myCode/AtomTheme")
    endif()
    if(NOT EXISTS "${ATOM_THEME_LOCAL}/CMakeLists.txt" AND EXISTS "$ENV{HOME}/source/AtomTheme/CMakeLists.txt")
        set(ATOM_THEME_LOCAL "$ENV{HOME}/source/AtomTheme")
    elseif(NOT EXISTS "${ATOM_THEME_LOCAL}/CMakeLists.txt" AND EXISTS "D:/myCode/AtomTheme/CMakeLists.txt")
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

    set(MINIBUSS_LOCAL "$ENV{HOME}/source/minibuss")
    if(NOT EXISTS "${MINIBUSS_LOCAL}/CMakeLists.txt" AND EXISTS "D:/source/minibuss/CMakeLists.txt")
        set(MINIBUSS_LOCAL "D:/source/minibuss")
    endif()
    if(EXISTS "${MINIBUSS_LOCAL}/CMakeLists.txt")
        message(STATUS "minibuss found at ${MINIBUSS_LOCAL} — using local checkout")
        set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
        add_subdirectory("${MINIBUSS_LOCAL}"
                         "${CMAKE_BINARY_DIR}/minibuss"
                         EXCLUDE_FROM_ALL)
        set(_minibuss_dir "${MINIBUSS_LOCAL}")
    else()
        message(STATUS "minibuss not found locally — fetching from GitHub")
        set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            minibuss
            GIT_REPOSITORY  https://github.com/guthmanny/minibuss.git
            GIT_TAG         main
            GIT_SHALLOW     TRUE
        )
        FetchContent_MakeAvailable(minibuss)
        set(_minibuss_dir "${minibuss_SOURCE_DIR}")
    endif()
    set(MINIBUSS_DIR "${_minibuss_dir}" PARENT_SCOPE)
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
        minibuss_plugin
        NuDSP::nudsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
    )
endfunction()
