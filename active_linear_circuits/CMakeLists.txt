cmake_minimum_required(VERSION 3.22)
project(Ds1OpampAcTracer VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

# Parent of measurements/ (e.g. D:/myCode) holds sibling checkouts.
get_filename_component(_DEPS_ROOT "${CMAKE_SOURCE_DIR}/../.." ABSOLUTE)

# ---------------------------------------------------------------------------
# JUCE
# ---------------------------------------------------------------------------
set(_juce_candidates
    "${_DEPS_ROOT}/JUCE"
    "D:/source/JUCE"
    "${CMAKE_SOURCE_DIR}/../../../source/JUCE"
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
set(BUILD_SSMEL_ENGINE OFF CACHE BOOL "" FORCE)
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
    PRODUCT_NAME "DS-1 White Box AC Tracer"
    VERSION 0.1.0
    COMPANY_NAME "nux"
)

juce_generate_juce_header(Ds1OpampAcTracer)

target_sources(Ds1OpampAcTracer PRIVATE
    ../common/SchematicSignalOutput.cpp
    ../common/SchematicSignalOutput.h
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    Source/CircuitSchematicPanel.cpp
    Source/CircuitSchematicPanel.h
    Source/SchematicSvgAdapter.h
    Source/SchematicComponentValues.cpp
    Source/SchematicComponentValues.h
    Source/SchematicComponentApply.cpp
    Source/SchematicComponentApply.h
    Source/Ds1OpampAcMath.cpp
    Source/Ds1OpampAcMath.h
    Source/SineWavePreviewEngine.cpp
    Source/SineWavePreviewEngine.h
    Source/Ds1OpampAcPanel.cpp
    Source/Ds1OpampAcPanel.h
)

target_compile_definitions(Ds1OpampAcTracer PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="DS-1 White Box AC Tracer"
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

# Must link after juce_recommended_warning_flags so -Wno-* wins on the compile line.
add_library(ds1_ac_tracer_warning_quiet INTERFACE)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(ds1_ac_tracer_warning_quiet INTERFACE
        -Wno-switch-enum
        -Wno-float-equal
        -Wno-overloaded-virtual
        -Wno-deprecated-declarations
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
    ds1_ac_tracer_warning_quiet
)

target_include_directories(Ds1OpampAcTracer PRIVATE
    "${CMAKE_SOURCE_DIR}/../common"
    "${MUDSP_ROOT}/core")

file(TO_CMAKE_PATH "${MUDSP_ROOT}" MUDSP_ROOT_CMAKE)
target_compile_definitions(Ds1OpampAcTracer PRIVATE MUDSP_ROOT="${MUDSP_ROOT_CMAKE}")

# JUCE writes Release builds under artefacts/Release/. The launcher used in this
# tree is the copy directly under artefacts/.
add_custom_command(TARGET Ds1OpampAcTracer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Ds1OpampAcTracer>
            "${CMAKE_BINARY_DIR}/Ds1OpampAcTracer_artefacts/$<TARGET_FILE_NAME:Ds1OpampAcTracer>"
    VERBATIM)

enable_testing()
add_executable(angry_charlie_chain_metadata_test Tests/angry_charlie_chain_metadata_test.cpp)
target_link_libraries(angry_charlie_chain_metadata_test PRIVATE NuDSP::nudsp)
target_include_directories(angry_charlie_chain_metadata_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(angry_charlie_chain_metadata_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME angry_charlie_chain_metadata COMMAND $<TARGET_FILE:angry_charlie_chain_metadata_test>)

add_executable(crunch_box_chain_metadata_test Tests/crunch_box_chain_metadata_test.cpp)
target_link_libraries(crunch_box_chain_metadata_test PRIVATE NuDSP::nudsp)
target_include_directories(crunch_box_chain_metadata_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(crunch_box_chain_metadata_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME crunch_box_chain_metadata COMMAND $<TARGET_FILE:crunch_box_chain_metadata_test>)

add_executable(guvnor_chain_metadata_test Tests/guvnor_chain_metadata_test.cpp)
target_link_libraries(guvnor_chain_metadata_test PRIVATE NuDSP::nudsp)
target_include_directories(guvnor_chain_metadata_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(guvnor_chain_metadata_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME guvnor_chain_metadata COMMAND $<TARGET_FILE:guvnor_chain_metadata_test>)

add_executable(odr1_drive_operator_test Tests/odr1_drive_operator_test.cpp)
target_link_libraries(odr1_drive_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(odr1_drive_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(odr1_drive_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME odr1_drive_operator COMMAND $<TARGET_FILE:odr1_drive_operator_test>)

add_executable(odr1_clipper_operator_test Tests/odr1_clipper_operator_test.cpp)
target_link_libraries(odr1_clipper_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(odr1_clipper_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(odr1_clipper_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME odr1_clipper_operator COMMAND $<TARGET_FILE:odr1_clipper_operator_test>)

add_executable(odr1_tone_operator_test Tests/odr1_tone_operator_test.cpp)
target_link_libraries(odr1_tone_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(odr1_tone_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(odr1_tone_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME odr1_tone_operator COMMAND $<TARGET_FILE:odr1_tone_operator_test>)

add_executable(odr1_postamp_operator_test Tests/odr1_postamp_operator_test.cpp)
target_link_libraries(odr1_postamp_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(odr1_postamp_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(odr1_postamp_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME odr1_postamp_operator COMMAND $<TARGET_FILE:odr1_postamp_operator_test>)

add_executable(beod_preamp_operator_test Tests/beod_preamp_operator_test.cpp)
target_link_libraries(beod_preamp_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(beod_preamp_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(beod_preamp_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME beod_preamp_operator COMMAND $<TARGET_FILE:beod_preamp_operator_test>)

add_executable(beod_tight_operator_test Tests/beod_tight_operator_test.cpp)
target_link_libraries(beod_tight_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(beod_tight_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(beod_tight_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME beod_tight_operator COMMAND $<TARGET_FILE:beod_tight_operator_test>)

add_executable(fa1_buffer_operator_test Tests/fa1_buffer_operator_test.cpp)
target_link_libraries(fa1_buffer_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(fa1_buffer_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(fa1_buffer_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME fa1_buffer_operator COMMAND $<TARGET_FILE:fa1_buffer_operator_test>)

add_executable(fa1_volume_operator_test Tests/fa1_volume_operator_test.cpp)
target_link_libraries(fa1_volume_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(fa1_volume_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(fa1_volume_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME fa1_volume_operator COMMAND $<TARGET_FILE:fa1_volume_operator_test>)

add_executable(fa1_eq_operator_test Tests/fa1_eq_operator_test.cpp)
target_link_libraries(fa1_eq_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(fa1_eq_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(fa1_eq_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME fa1_eq_operator COMMAND $<TARGET_FILE:fa1_eq_operator_test>)

add_executable(bad_monkey_low_operator_test Tests/bad_monkey_low_operator_test.cpp)
target_link_libraries(bad_monkey_low_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(bad_monkey_low_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(bad_monkey_low_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME bad_monkey_low_operator COMMAND $<TARGET_FILE:bad_monkey_low_operator_test>)

add_executable(dyna_comp_core_operator_test Tests/dyna_comp_core_operator_test.cpp)
target_link_libraries(dyna_comp_core_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(dyna_comp_core_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(dyna_comp_core_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME dyna_comp_core_operator COMMAND $<TARGET_FILE:dyna_comp_core_operator_test>)

add_executable(dirty_shirley_tight_operator_test Tests/dirty_shirley_tight_operator_test.cpp)
target_link_libraries(dirty_shirley_tight_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(dirty_shirley_tight_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(dirty_shirley_tight_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME dirty_shirley_tight_operator COMMAND $<TARGET_FILE:dirty_shirley_tight_operator_test>)

add_executable(dirty_shirley_mid_operator_test Tests/dirty_shirley_mid_operator_test.cpp)
target_link_libraries(dirty_shirley_mid_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(dirty_shirley_mid_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(dirty_shirley_mid_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME dirty_shirley_mid_operator COMMAND $<TARGET_FILE:dirty_shirley_mid_operator_test>)

add_executable(beod_pregain_operator_test Tests/beod_pregain_operator_test.cpp)
target_link_libraries(beod_pregain_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(beod_pregain_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(beod_pregain_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME beod_pregain_operator COMMAND $<TARGET_FILE:beod_pregain_operator_test>)

add_executable(beod_postgain_operator_test Tests/beod_postgain_operator_test.cpp)
target_link_libraries(beod_postgain_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(beod_postgain_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(beod_postgain_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME beod_postgain_operator COMMAND $<TARGET_FILE:beod_postgain_operator_test>)

add_executable(beod_trim_operator_test Tests/beod_trim_operator_test.cpp)
target_link_libraries(beod_trim_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(beod_trim_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(beod_trim_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME beod_trim_operator COMMAND $<TARGET_FILE:beod_trim_operator_test>)

add_executable(beod_clipper_operator_test Tests/beod_clipper_operator_test.cpp)
target_link_libraries(beod_clipper_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(beod_clipper_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(beod_clipper_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME beod_clipper_operator COMMAND $<TARGET_FILE:beod_clipper_operator_test>)

add_executable(beod_bass_operator_test Tests/beod_bass_operator_test.cpp)
target_link_libraries(beod_bass_operator_test PRIVATE NuDSP::nudsp)
target_include_directories(beod_bass_operator_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(beod_bass_operator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME beod_bass_operator COMMAND $<TARGET_FILE:beod_bass_operator_test>)

add_executable(beod_chain_metadata_test Tests/beod_chain_metadata_test.cpp)
target_link_libraries(beod_chain_metadata_test PRIVATE NuDSP::nudsp)
target_include_directories(beod_chain_metadata_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(beod_chain_metadata_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME beod_chain_metadata COMMAND $<TARGET_FILE:beod_chain_metadata_test>)

add_executable(dirty_shirley_chain_metadata_test Tests/dirty_shirley_chain_metadata_test.cpp)
target_link_libraries(dirty_shirley_chain_metadata_test PRIVATE NuDSP::nudsp)
target_include_directories(dirty_shirley_chain_metadata_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(dirty_shirley_chain_metadata_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME dirty_shirley_chain_metadata COMMAND $<TARGET_FILE:dirty_shirley_chain_metadata_test>)

add_executable(bad_monkey_chain_metadata_test Tests/bad_monkey_chain_metadata_test.cpp)
target_link_libraries(bad_monkey_chain_metadata_test PRIVATE NuDSP::nudsp)
target_include_directories(bad_monkey_chain_metadata_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(bad_monkey_chain_metadata_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME bad_monkey_chain_metadata COMMAND $<TARGET_FILE:bad_monkey_chain_metadata_test>)

add_executable(dyna_comp_chain_metadata_test Tests/dyna_comp_chain_metadata_test.cpp)
target_link_libraries(dyna_comp_chain_metadata_test PRIVATE NuDSP::nudsp)
target_include_directories(dyna_comp_chain_metadata_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(dyna_comp_chain_metadata_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME dyna_comp_chain_metadata COMMAND $<TARGET_FILE:dyna_comp_chain_metadata_test>)

add_executable(odr1_chain_metadata_test Tests/odr1_chain_metadata_test.cpp)
target_link_libraries(odr1_chain_metadata_test PRIVATE NuDSP::nudsp)
target_include_directories(odr1_chain_metadata_test PRIVATE "${MUDSP_ROOT}/include" "${MUDSP_ROOT}/core")
set_target_properties(odr1_chain_metadata_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
add_test(NAME odr1_chain_metadata COMMAND $<TARGET_FILE:odr1_chain_metadata_test>)
