# Audio Effect Framework

Shared infrastructure for JUCE + AtomTheme + kbuss audio effect plugins.

## Layout

```
measurements/
  audio_effect_framework/     ← edit here; all plugins pick up changes on rebuild
    Source/
    cmake/AudioEffectFramework.cmake
  template_audio_effects/     ← minimal example plugin
  ds1/                        ← Boss DS-1 example
  moog_ladder/                ← Moog Ladder with custom editor
```

## Create a new plugin

1. Copy `template_audio_effects/` to `MyEffect/`
2. Edit `CMakeLists.txt`: change `project()`, `juce_add_plugin()`, `PLUGIN_CODE`, `PRODUCT_NAME`
3. Subclass `AudioEffectFrameworkProcessor`:
   - Override `createEffectEngine()` to return `MiddleProcessorEffectEngine(uid, name, instance)` (or a custom `KbussEffectEngine` subclass if needed)
   - Add effect parameters in the constructor
   - Override `updateCustomEffectParameters()` to push values to the middle processor via `getKbussEngine().middleProcessorId()`
   - Override `createEditor()` (use `AudioEffectFrameworkEditor` or subclass it)
   - Override `getName()`, `acceptsMidi()`, etc. using `JucePlugin_*` macros
   - For mono guitar pedals: override `processBlock()` with `aef::mixBufferToMonoDual` / `aef::duplicateMonoToStereoOutput`, and override `bypassNoiseGateOnStartup()` to return `true`

## CMake snippet

```cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../audio_effect_framework/cmake")
include(AudioEffectFramework)
aef_setup_dependencies()
add_subdirectory(../audio_effect_framework ${CMAKE_BINARY_DIR}/audio_effect_framework)
juce_add_plugin(MyEffect ...)
juce_generate_juce_header(MyEffect)
target_sources(MyEffect PRIVATE Source/PluginProcessor.cpp)
aef_apply_plugin_target(MyEffect)
```

`aef_setup_dependencies()` resolves MuDSP and adds kbuss from a sibling checkout (`../../kbuss`). Override with `-DMUDSP_ROOT=` / `-DKBUSS_ROOT=` when needed.

`aef_apply_plugin_target()` links the shared editor, standalone entry (`AefStandaloneMain.cpp`), and common compile settings — no per-plugin `StandaloneMain.cpp` needed.

## What lives in the framework

- `AudioEffectFrameworkProcessor` / `AudioEffectFrameworkEditor`
- Gain → NoiseGate → Upsampler → [middle] → Downsampler → Level kbuss chain (`KbussEffectEngine`)
- `MiddleProcessorEffectEngine` — configurable middle plugin UID/name/instance
- `AefAudioUtils` — parameter defaults, mono mix / stereo duplicate helpers
- `AefStandaloneMain.cpp` — shared Standalone app (native title bar, JACK capture routing)
- Header/Footer (QUALITY: STANDARD 2× / HIGH 4× / ULTRA 8×), Settings panels, Tuner, Spectrum, Calibration (Ki/Ko)
- `PluginParameter`, meter display, JACK routing helpers

## What stays in each plugin

- `PluginProcessor` subclass (effect parameters + kbuss UID wiring; UIDs use `com.kbuss.*`)
- Plugin-specific editor (optional; default is `AudioEffectFrameworkEditor`)
- Plugin identity in `CMakeLists.txt`

## Legacy aliases

`MinibussEffectEngine`, `getMinibussEngine()`, and `MinibussEffectEngine.h` remain as aliases for older plugin code. New code should use `KbussEffectEngine` / `getKbussEngine()`.
