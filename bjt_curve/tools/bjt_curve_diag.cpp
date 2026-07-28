#include "BjtCurveMath.h"

#include <cstdio>

int main()
{
    const auto ibValues = bjt_curve::buildIbSweepValues({ 5, 1.0e-6f, 1.5e-6f });
    const float vceMax = 5.0f;

    auto families = bjt_curve::buildIcVsVceFamily(NX_BJT_NPN, bjt_curve::CircuitKind::CommonEmitter, ibValues, 32, vceMax);
    auto overlay = bjt_curve::buildCommonEmitterDcLoadLineOverlay(NX_BJT_NPN, ibValues, families, vceMax);
    auto range = bjt_curve::computeAxisRangeForFamily(families);
    bjt_curve::expandAxisRangeForOverlay(range, overlay, vceMax);
    bjt_curve::clipOverlayLoadLinesToAxis(overlay, range);

    std::printf("=== axis: X[%.2f,%.2f] V  Y[0,%.1f uA] ===\n",
                range.minX, range.maxX, range.maxY * 1e6f);

    if (!overlay.loadLines.empty() && overlay.loadLines.front().size() >= 2)
    {
        const auto& seg = overlay.loadLines.front();
        std::printf("load line: (%.3f, %.1f uA) -> (%.3f, %.1f uA)\n",
                    seg.front().first, seg.front().second * 1e6f,
                    seg.back().first, seg.back().second * 1e6f);
    }

    if (overlay.qPoint.valid)
        std::printf("Q: Vbe=%.4f V  Vce=%.4f V  Ib=%.2f uA  Ic=%.1f uA\n",
                    overlay.qPoint.vbe, overlay.qPoint.vce,
                    overlay.qPoint.ib * 1e6f, overlay.qPoint.ic * 1e6f);
    else
        std::printf("Q: not set (invalid DC)\n");

    for (size_t i = 0; i < ibValues.size(); ++i)
    {
        std::printf("\n--- Ib target %.1f uA ---\n", ibValues[i] * 1e6f);
        if (i < families.size() && !families[i].empty())
        {
            const auto& fam = families[i];
            std::printf("curve: n=%zu  vce[0]=%.3f ic[0]=%.1fuA  vce[-1]=%.3f ic[-1]=%.1fuA\n",
                        fam.size(), fam.front().first, fam.front().second * 1e6f,
                        fam.back().first, fam.back().second * 1e6f);
        }
        else
        {
            std::printf("curve: EMPTY\n");
        }
    }

    return 0;
}
