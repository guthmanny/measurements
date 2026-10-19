#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "AefJuceIncludes.h"

class AudioEffectFrameworkProcessor;
class AtomLookAndFeel;

/** Resolves atomtheme-gfx-packs/<gfx_key> (bundled .kbplug/gfx or GFX_PACKS_ROOT). */
[[nodiscard]] juce::File resolveGfxPackDirectory(const AudioEffectFrameworkProcessor& processor);

/** Starts background decode of pack art so the editor ctor can overlap UI setup. */
void prefetchGfxPackArt(const AudioEffectFrameworkProcessor& processor,
                        const juce::String& profile = "desktop");

/** Skeuomorphic pedal face from a gfx pack panel.json (replaces standard atom sliders).
    Stay hidden until the host has applied this panel's preferred size. Painting into
    the previous model's window height clips the pedal at that bottom edge, which moves
    depending on the last model and VIEW zoom — it is not the back/stomp artwork seam. */
class GfxPedalPanel final : public juce::Component
{
public:
    GfxPedalPanel(AudioEffectFrameworkProcessor& processor, AtomLookAndFeel& lookAndFeel);
    ~GfxPedalPanel() override;

    [[nodiscard]] bool load(const juce::File& packDir, const juce::String& profile = "desktop");
    [[nodiscard]] bool isLoadedFrom(const juce::File& packDir) const noexcept;
    [[nodiscard]] bool isFaceReady() const noexcept { return faceReady_; }
    void refreshFromProcessor();

    [[nodiscard]] int preferredWidth() const noexcept { return preferredWidth_; }
    [[nodiscard]] int preferredHeight() const noexcept { return preferredHeight_; }

    void paint(juce::Graphics& g) override;
    void resized() override;

    void syncBypassIndicator();
    std::function<void()> onPreferredSizeChanged;

private:
    struct ControlSpec
    {
        juce::Component* widget = nullptr;
        float x = 0.f;
        float y = 0.f;
        float w = 0.f;
        float h = 0.f;
    };

    void clearControls();
    bool loadPanel(const juce::File& profileDir, const juce::var& panel);
    juce::Image loadArt(const juce::File& profileDir, const juce::String& relative) const;
    juce::Image loadKnockedOutArt(const juce::File& profileDir, const juce::String& relative) const;
    void finishPendingLoad();
    void fitToArtwork();
    void flattenOpaqueFace();
    void rebuildPresentation();
    void notifyPreferredSizeChanged();
    [[nodiscard]] juce::Rectangle<float> fittedArtBounds() const noexcept;
    [[nodiscard]] bool hostBypassed() const;
    void setHostBypassed(bool bypassed);

    AudioEffectFrameworkProcessor& processor_;
    AtomLookAndFeel& atomLookAndFeel_;
    juce::Image background_;
    juce::Image presentation_;
    juce::OwnedArray<juce::Component> widgets_;
    std::vector<ControlSpec> layout_;
    std::vector<std::pair<juce::String, juce::Component*>> boundKnobs_;
    std::vector<std::pair<juce::String, juce::Component*>> boundSwitches_;
    juce::Component* bypassLight_ = nullptr;
    juce::File loadedPackDir_;
    juce::File pendingProfileDir_;
    juce::var pendingPanel_;
    uint32_t loadGeneration_ = 0;
    bool faceReady_ = false;
    float logicalW_ = 800.f;
    float logicalH_ = 700.f;
    int preferredWidth_ = 800;
    int preferredHeight_ = 700;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GfxPedalPanel)
};
