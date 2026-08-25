#pragma once

#include <cstddef>

struct CamelEffectEntry
{
    const char* uid;
    const char* displayName;
};

inline constexpr CamelEffectEntry kCamelEffects[] = {
    {"com.kbuss.nudsp.camel.comp", "Compressor"},
    {"com.kbuss.nudsp.camel.limiter", "Limiter"},
    {"com.kbuss.nudsp.camel.overdrive", "Overdrive"},
    {"com.kbuss.nudsp.camel.distortion", "Distortion"},
    {"com.kbuss.nudsp.camel.eq", "Stereo EQ"},
    {"com.kbuss.nudsp.camel.spectrum", "Spectrum"},
    {"com.kbuss.nudsp.camel.stereo-chorus", "Stereo Chorus"},
    {"com.kbuss.nudsp.camel.hexa-chorus", "Hexa Chorus"},
    {"com.kbuss.nudsp.camel.tremolo-chorus", "Tremolo Chorus"},
    {"com.kbuss.nudsp.camel.space-d", "Space-D"},
    {"com.kbuss.nudsp.camel.stereo-flanger", "Stereo Flanger"},
    {"com.kbuss.nudsp.camel.step-flanger", "Step Flanger"},
    {"com.kbuss.nudsp.camel.phaser", "Phaser"},
    {"com.kbuss.nudsp.camel.auto-wah", "Auto-Wah"},
    {"com.kbuss.nudsp.camel.rotary", "Rotary"},
    {"com.kbuss.nudsp.camel.stereo-delay", "Stereo Delay"},
    {"com.kbuss.nudsp.camel.modulation-delay", "Modulation Delay"},
    {"com.kbuss.nudsp.camel.triple-tap-delay", "Triple-Tap Delay"},
    {"com.kbuss.nudsp.camel.quadruple-tap-delay", "Quadruple-Tap Delay"},
    {"com.kbuss.nudsp.camel.time-control-delay", "Time-Control Delay"},
    {"com.kbuss.nudsp.camel.two-voice-pitch-shifter", "2 Voice Pitch Shifter"},
    {"com.kbuss.nudsp.camel.fbk-pitch-shifter", "FBK Pitch Shifter"},
    {"com.kbuss.nudsp.camel.stereo-reverb", "Reverb"},
    {"com.kbuss.nudsp.camel.gate-reverb", "Gate Reverb"},
};

inline constexpr std::size_t kCamelEffectCount = sizeof(kCamelEffects) / sizeof(kCamelEffects[0]);
