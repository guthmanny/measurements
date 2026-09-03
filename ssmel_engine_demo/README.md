# SSMEL Engine Demo

Standalone JUCE client that shows **Dream Editor XDI → `nx_pcm_sample_map_f32_t` → MuDSP `ssmel_engine`**.

The demo exists to prove the file is heard as written. Amplifier, Envelope, Filter, and loop points come from the XDI — not from engine playability presets.

App structure matches [bleach](../../bleach) / [basic_synth](../basic_synth):

- AtomTheme UI (`AtomLookAndFeel`, settings dialog, on-screen keyboard)
- Custom `StandaloneMain` (no audio-effect JACK restart path)
- Client-owned `nx_pcm_sample_map_f32_t` — engine does not parse XDI/WAV

## What it shows

1. `SsmelDemoMap` parses `Piano2ry.XDI` / `Ep2.XDI` and their WAVs into `nx_pcm_sample_map_f32_t`.
2. Dropdown **Piano** / **EP** swaps the client-owned map. Resonance / harmonic resonance stay off.
3. Engine lifecycle: `create` → `prepare` → `set_sample_map` → MIDI `note_on/off` → `render_stereo`.
4. Phase 3 controls: output gain and SVF cutoff.

Sound bank: `example_sound_sources/sources/` (Dream TutorialBank XDI + WAV).

| Bank | XDI | Notes |
|------|-----|-------|
| Piano | `Piano2ry.XDI` | 9 splits, `LoopType=Forward`, no release layers. From MIDI 78 the amp EG is 5-point (sustain = point 4 = 0). |
| EP | `Ep2.XDI` | `fmc2`–`fmc7`, same loop / layer layout. Env1 is 4-point (sustain = point 3 = 0). |

Both banks: Env2 `SustainPoint=-1` (oneshot). Held sound is the FORWARD loop + Env1, not a filter-EG gate.

Integration contract (envelope, Rate→ms, velocity, filter, loop): [MuDSP/docs/SSMEL_ENGINE_INTEGRATION.md](../../MuDSP/docs/SSMEL_ENGINE_INTEGRATION.md).

Parser: `Source/SsmelDemoMap.cpp` (`parseEnvelope`, `xdiRateToMs`, `parseAmpVel`). Rate→ms is a linear placeholder (`T(0)=8000`, `T(0.99)=10`, `T(≥1.27)=1`); swap only those endpoints after measurement.

## Build

```bash
cd ~/myCode/measurements/ssmel_engine_demo
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target SsmelEngineDemo_Standalone
./build/SsmelEngineDemo_artefacts/Release/Standalone/SsmelEngineDemo
```

Requires sibling checkouts: `MuDSP` (with `BUILD_SSMEL_ENGINE=ON`), `kbuss`, `AtomTheme`, and JUCE 8+. Local MuDSP is pulled via kbuss (`../MuDSP`).
