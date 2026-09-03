#pragma once

#include "nusynth/pcm_sample_map_f32.h"

/** EG times from XDI Envelope1/2 (User Guide §5.4). Rate→ms is a placeholder. */
struct SsmelXdiVoicing
{
    double eg1AttackMs = 1.0;
    double eg1HoldMs = 0.0;
    double eg1DecayMs = 10.0;
    double eg1BreakMs = 0.0;
    float eg1BreakLevel = 0.0f;
    float eg1Sustain = 1.0f;
    double eg1ReleaseMs = 500.0;
    bool eg1Oneshot = false;
    double eg2AttackMs = 1.0;
    double eg2HoldMs = 0.0;
    double eg2DecayMs = 700.0;
    double eg2BreakMs = 0.0;
    float eg2BreakLevel = 0.0f;
    float eg2Sustain = 0.32f;
    double eg2ReleaseMs = 180.0;
    bool eg2Oneshot = true;
};

/** Client-owned maps built from Dream Editor Ep2.XDI / Piano2ry.XDI. */
class SsmelDemoMap
{
public:
    SsmelDemoMap() = default;
    ~SsmelDemoMap();

    SsmelDemoMap(const SsmelDemoMap&) = delete;
    SsmelDemoMap& operator=(const SsmelDemoMap&) = delete;

    bool load();
    void reset();

    /** 0 = Piano (Piano2ry), 1 = EP (Ep2). */
    nx_pcm_sample_map_f32_t* map(int presetIndex) noexcept;
    const nx_pcm_sample_map_f32_t* map(int presetIndex) const noexcept;
    const SsmelXdiVoicing& voicing(int presetIndex) const noexcept;

private:
    nx_pcm_sample_map_f32_t* pianoMap_ = nullptr;
    nx_pcm_sample_map_f32_t* epMap_ = nullptr;
    SsmelXdiVoicing pianoVoicing_{};
    SsmelXdiVoicing epVoicing_{};
};
