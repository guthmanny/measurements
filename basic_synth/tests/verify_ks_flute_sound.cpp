#include "KbussSynthEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <span>
#include <vector>

int main()
{
    KbussSynthEngine engine;
    engine.prepare (48000.f, 128, SynthInstrument::KsFlute);

    if (! engine.isReady())
    {
        std::fprintf (stderr, "FAIL: KbussSynthEngine (KS Flute) not ready\n");
        return 1;
    }

    engine.setParamDomain (engine.synthId(), "gain", 1.0f);
    engine.setParamDomain (engine.synthId(), "output_boost", 1.5f);
    engine.setParamDomain (engine.synthId(), "noise", 0.01f);
    engine.setParamDomain (engine.synthId(), "feedback", 1.1f);
    engine.setParamDomain (engine.synthId(), "eg1_attack", 3.0f);
    engine.sendNoteOn (60, 0.85f);

    std::vector<float> left (128), right (128);
    float* outputs[] = { left.data(), right.data() };

    float peak = 0.f;
    for (int block = 0; block < 512; ++block)
    {
        engine.process (std::span<float* const> (outputs, 2), 128);
        for (int i = 0; i < 128; ++i)
        {
            peak = std::max (peak, std::abs (left[i]));
            peak = std::max (peak, std::abs (right[i]));
        }
    }

    engine.sendNoteOff (60, 0.f);

    std::printf ("verify_ks_flute_sound: peak=%.6f\n", peak);

    if (peak < 1.0e-4f)
    {
        std::fprintf (stderr, "FAIL: no audio after MIDI note on (peak=%.6f)\n", peak);
        return 1;
    }

    std::printf ("PASS: KS Flute MIDI note produced non-silent output\n");
    return 0;
}
