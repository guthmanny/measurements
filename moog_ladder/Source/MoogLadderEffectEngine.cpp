#include "MoogLadderEffectEngine.h"

bool MoogLadderEffectEngine::installMiddleProcessors(const ProcessorCreateFn& create)
{
  const auto id = create("com.minibuss.nudsp.ssmel.moog_ladder", "Moog Ladder", "moog_ladder");
  middleProcessorId_ = id;
  return id != minibuss::kInvalidObjectId;
}
