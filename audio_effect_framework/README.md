# Audio Effect Framework

Shared infrastructure for JUCE + AtomTheme + minibuss audio effect plugins.

## Layout

```
measurements/
  audio_effect_framework/     ← edit here; all plugins pick up changes on rebuild
    Source/
    cmake/AudioEffectFramework.cmake
  template_audio_effects/     ← minimal example plugin
  Chorus/                     ← (future) link framework + chorus-only code
```

## Create a new plugin

1. Copy `template_audio_effects/` to `MyEffect/`
2. Edit `CMakeLists.txt`: change `project()`, `juce_add_plugin()`, `PLUGIN_CODE`, `PRODUCT_NAME`
3. Subclass `AudioEffectFrameworkProcessor`:
   - Add effect parameters in the constructor
   - Override `updateCustomEffectParameters()` to push values to minibuss
   - Override `createEditor()` (use `AudioEffectFrameworkEditor` or subclass it)
   - Override `getName()`, `acceptsMidi()`, etc. using `JucePlugin_*` macros
4. Insert your processor in `MinibussEffectEngine::prepare()` (or extend the engine in your plugin)

## CMake snippet

```cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../audio_effect_framework/cmake")
include(AudioEffectFramework)
aef_setup_dependencies()
add_subdirectory(../audio_effect_framework ${CMAKE_BINARY_DIR}/audio_effect_framework)
juce_add_plugin(MyEffect ...)
juce_generate_juce_header(MyEffect)
target_sources(MyEffect PRIVATE Source/PluginProcessor.cpp Source/StandaloneMain.cpp)
aef_apply_plugin_target(MyEffect)
```

## What lives in the framework

- `AudioEffectFrameworkProcessor` / `AudioEffectFrameworkEditor`
- Gain → NoiseGate → Upsampler → [middle] → Downsampler → Level minibuss chain (`MinibussEffectEngine`)
- Header/Footer (QUALITY: STANDARD 2× / HIGH 4× / ULTRA 8×), Settings panels, Tuner, Spectrum, Calibration
- `PluginParameter`, meter display, JACK routing helpers

## What stays in each plugin

- `PluginProcessor` subclass (effect DSP + parameters)
- `StandaloneMain.cpp` (uses `JucePlugin_Name`)
- Plugin identity in `CMakeLists.txt`
