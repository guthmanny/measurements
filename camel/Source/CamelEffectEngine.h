#pragma once

#include "CamelCatalog.h"
#include "KbussEffectEngine.h"

/** kbuss chain with a swappable CAMEL product-effect middle slot. */
class CamelEffectEngine final : public KbussEffectEngine
{
public:
    CamelEffectEngine() = default;

    void setEffectIndex(int index) noexcept;
    [[nodiscard]] int effectIndex() const noexcept { return effectIndex_; }

    void reprepare(float sampleRate, std::uint32_t maxBlockSize);

protected:
    bool installMiddleProcessors(const ProcessorCreateFn& create) override;

private:
    int effectIndex_ = 0;
};
