#include "KbussSynthEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <span>
#include <vector>

int main()
{
    KbussSynthEngine engine;
    engine.prepare (48000.f, 128);

    if (! engine.isReady())
    {
        std::fprintf (stderr, "FAIL: KbussSynthEngine not ready\n");
        return 1;
    }

    engine.setParamDomain (engine.synthId(), "gain", 0.5f);
    engine.sendNoteOn (60, 0.8f);

    std::vector<float> left (128), right (128);
    float* outputs[] = { left.data(), right.data() };

    float peak = 0.f;
    for (int block = 0; block < 256; ++block)
    {
        engine.process (std::span<float* const> (outputs, 2), 128);
        for (int i = 0; i < 128; ++i)
        {
            peak = std::max (peak, std::abs (left[i]));
            peak = std::max (peak, std::abs (right[i]));
        }
    }

    engine.sendNoteOff (60, 0.f);

    std::printf ("verify_sound: peak=%.6f\n", peak);

    if (peak < 1.0e-4f)
    {
        std::fprintf (stderr, "FAIL: no audio after MIDI note on (peak=%.6f)\n", peak);
        return 1;
    }

    std::printf ("PASS: MIDI note produced non-silent output\n");
    return 0;
}
