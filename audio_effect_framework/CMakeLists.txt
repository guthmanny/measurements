# audio_effect_framework — shared infrastructure for AtomTheme + minibuss audio effects.
#
# Usage from a plugin project:
#   list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../audio_effect_framework/cmake")
#   include(AudioEffectFramework)
#   aef_setup_dependencies()
#   add_subdirectory(../audio_effect_framework ${CMAKE_BINARY_DIR}/audio_effect_framework)
#   juce_add_plugin(...)
#   target_link_libraries(MyPlugin PRIVATE audio_effect_framework ...)
#   aef_apply_plugin_target(MyPlugin)

if(TARGET audio_effect_framework)
    return()
endif()

set(AEF_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")

add_library(audio_effect_framework STATIC
    Source/AudioEffectFrameworkProcessor.cpp
    Source/AudioEffectFrameworkProcessor.h
    Source/AudioEffectFrameworkEditor.h
    Source/MinibussEffectEngine.cpp
    Source/MinibussEffectEngine.h
    Source/MiddleProcessorEffectEngine.cpp
    Source/MiddleProcessorEffectEngine.h
    Source/AefAudioUtils.h
    Source/EffectHeaderComponent.cpp
    Source/EffectHeaderComponent.h
    Source/EffectFooterComponent.cpp
    Source/EffectFooterComponent.h
    Source/TunerOverlayComponent.cpp
    Source/TunerOverlayComponent.h
    Source/SpectrumOverlayComponent.cpp
    Source/SpectrumOverlayComponent.h
    Source/SpectrumCurveComponent.cpp
    Source/SpectrumCurveComponent.h
    Source/SpectrumAnalyzer.h
    Source/TunerDetector.h
    Source/AppSettingsPanel.cpp
    Source/AppSettingsPanel.h
    Source/NoiseGateSettingsPanel.cpp
    Source/NoiseGateSettingsPanel.h
    Source/PeakDisplaySettingsPanel.cpp
    Source/PeakDisplaySettingsPanel.h
    Source/CalibrationSettingsPanel.cpp
    Source/CalibrationSettingsPanel.h
    Source/EffectTopologyChainComponent.cpp
    Source/EffectTopologyChainComponent.h
    Source/EffectTopologyModule.h
    Source/TopologySettingsPanel.cpp
    Source/TopologySettingsPanel.h
    Source/OversamplingSettingsPanel.cpp
    Source/OversamplingSettingsPanel.h
    Source/MeterDisplayUtils.h
    Source/CompressorCurveComponent.cpp
    Source/CompressorCurveComponent.h
    Source/CompressorKnee.h
    Source/PluginParameter.h
    Source/JackCaptureRouting.cpp
    Source/JackCaptureRouting.h
    ${ATOM_COLLECTIONS_APP_DIR}/AudioSettingsPanel.cpp
    ${ATOM_COLLECTIONS_APP_DIR}/AudioSettingsPanel.h
)

target_include_directories(audio_effect_framework
    PUBLIC
        "${AEF_SOURCE_DIR}/Source"
    PRIVATE
        "${ATOM_COLLECTIONS_APP_DIR}"
        "${MINIBUSS_DIR}/include"
        "${MINIBUSS_DIR}/src"
)

target_compile_features(audio_effect_framework PUBLIC cxx_std_20)

target_link_libraries(audio_effect_framework
    PUBLIC
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_atom_theme
        atom_theme_core_impl
        minibuss_plugin
        NuDSP::nudsp
)

if(WIN32)
    target_compile_definitions(audio_effect_framework PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(NOT MSVC)
    target_compile_options(audio_effect_framework PRIVATE
        -Wno-switch-enum
        -Wno-overloaded-virtual
    )
endif()

option(AEF_BUILD_STANDALONE "Compile standalone-only settings UI into the framework" ON)
if(AEF_BUILD_STANDALONE)
    target_compile_definitions(audio_effect_framework PUBLIC JucePlugin_Build_Standalone=1)
else()
    target_compile_definitions(audio_effect_framework PUBLIC JucePlugin_Build_Standalone=0)
endif()
