#include "GfxPedalPanel.h"

#include <atomic>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>

#include <juce_atom_theme/juce_atom_theme.h>

#include "AudioEffectFrameworkProcessor.h"
#include "KbussParamSliderUtils.h"
#include "KbussParamUiUtils.h"
#include "kbuss/gfx_packs.hpp"
#include "kbuss/processor.hpp"

namespace
{
constexpr double kPixelsForFullDrag = 200.0;

class SpriteKnob final : public juce::Component
{
public:
    SpriteKnob()
    {
        setOpaque(false);
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    }

    void setSpriteSheet(const juce::Image& sheet, int cols, int frames = 0)
    {
        spriteSheet_ = sheet;
        numCols_ = juce::jmax(1, cols);
        if (spriteSheet_.isValid() && numCols_ > 0)
        {
            frameSize_ = spriteSheet_.getWidth() / numCols_;
            const int numRows = (frameSize_ > 0) ? (spriteSheet_.getHeight() / frameSize_) : 1;
            totalFrames_ = (frames > 0) ? frames : (numCols_ * numRows);
        }
        repaint();
    }

    void setAngleRange(float startDeg, float endDeg) noexcept
    {
        angleStart_ = startDeg;
        angleEnd_ = endDeg;
        repaint();
    }

    void setRange(double minVal, double maxVal, double interval = 0.0)
    {
        range_ = juce::NormalisableRange<double>(minVal, maxVal, interval);
        currentValue_ = juce::jlimit(minVal, maxVal, currentValue_);
        defaultValue_ = juce::jlimit(minVal, maxVal, defaultValue_);
    }

    void setDefaultValue(double val) noexcept { defaultValue_ = val; }

    void setValue(double newValue, juce::NotificationType notification = juce::sendNotification)
    {
        newValue = range_.snapToLegalValue(juce::jlimit(range_.start, range_.end, newValue));
        if (juce::approximatelyEqual(currentValue_, newValue))
            return;
        currentValue_ = newValue;
        repaint();
        if (notification != juce::dontSendNotification && onValueChanged)
            onValueChanged(currentValue_);
    }

    [[nodiscard]] double getValue() const noexcept { return currentValue_; }

    std::function<void(double)> onValueChanged;

    void paint(juce::Graphics& g) override
    {
        if (! spriteSheet_.isValid() || frameSize_ <= 0)
            return;

        const int idx = frameIndex();
        const int col = idx % numCols_;
        const int row = idx / numCols_;
        g.drawImage(spriteSheet_,
                    0, 0, getWidth(), getHeight(),
                    col * frameSize_, row * frameSize_, frameSize_, frameSize_);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        dragStart_ = e.position;
        valueOnDown_ = currentValue_;
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        const double mouseDiff = static_cast<double>(dragStart_.y - e.position.y);
        const double newPos = range_.convertTo0to1(valueOnDown_) + mouseDiff * (1.0 / kPixelsForFullDrag);
        setValue(range_.convertFrom0to1(juce::jlimit(0.0, 1.0, newPos)));
    }

    void mouseDoubleClick(const juce::MouseEvent&) override { setValue(defaultValue_); }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        const double step = (range_.interval > 0.0) ? range_.interval
                                                    : (range_.end - range_.start) * 0.01;
        const double delta = (wheel.deltaY > 0.0f) ? step : -step;
        setValue(juce::jlimit(range_.start, range_.end, currentValue_ + delta));
    }

private:
    [[nodiscard]] int frameIndex() const noexcept
    {
        if (totalFrames_ <= 0)
            return 0;
        const double degPerFrame = 360.0 / static_cast<double>(totalFrames_);
        const int baseFrame = static_cast<int>(std::ceil(static_cast<double>(angleStart_) / degPerFrame));
        const int arcFrames = static_cast<int>(
            std::ceil(static_cast<double>(angleEnd_ - angleStart_) / degPerFrame));
        if (arcFrames <= 0)
            return baseFrame;
        const auto percent = range_.convertTo0to1(currentValue_);
        const int idx = baseFrame + static_cast<int>(percent * static_cast<double>(arcFrames - 1) + 0.5);
        return juce::jlimit(0, totalFrames_ - 1, idx);
    }

    juce::Image spriteSheet_;
    int numCols_ = 12;
    int frameSize_ = 0;
    int totalFrames_ = 120;
    float angleStart_ = 0.f;
    float angleEnd_ = 360.f;
    juce::NormalisableRange<double> range_ { 0.0, 1.0, 0.0 };
    double currentValue_ = 0.5;
    double defaultValue_ = 0.5;
    double valueOnDown_ = 0.5;
    juce::Point<float> dragStart_;
};

class ImageToggle final : public juce::ToggleButton
{
public:
    ImageToggle() : juce::ToggleButton(juce::String())
    {
        setOpaque(false);
        setWantsKeyboardFocus(false);
    }

    void setImages(const juce::Image& offImg, const juce::Image& onImg)
    {
        offImage_ = offImg;
        onImage_ = onImg;
        repaint();
    }

    void paintButton(juce::Graphics& g, bool, bool) override
    {
        const auto& img = getToggleState() ? onImage_ : offImage_;
        if (! img.isValid())
            return;
        const auto t = juce::RectanglePlacement(juce::RectanglePlacement::stretchToFit)
                           .getTransformToFit(img.getBounds().toFloat(), getLocalBounds().toFloat());
        g.drawImageTransformed(img, t, false);
    }

private:
    juce::Image offImage_, onImage_;
};

class ImagePushButton final : public juce::Button
{
public:
    ImagePushButton() : juce::Button(juce::String())
    {
        setOpaque(false);
        setWantsKeyboardFocus(false);
        setTriggeredOnMouseDown(true);
    }

    void setImages(const juce::Image& normalImg, const juce::Image& pressedImg)
    {
        normalImage_ = normalImg;
        pressedImage_ = pressedImg;
        repaint();
    }

    void paintButton(juce::Graphics& g, bool, bool isDown) override
    {
        const auto& img = (isDown && pressedImage_.isValid()) ? pressedImage_ : normalImage_;
        if (! img.isValid())
            return;
        const auto t = juce::RectanglePlacement(juce::RectanglePlacement::stretchToFit)
                           .getTransformToFit(img.getBounds().toFloat(), getLocalBounds().toFloat());
        g.drawImageTransformed(img, t, false);
    }

private:
    juce::Image normalImage_, pressedImage_;
};

/** Rotary switch drawn on the artwork: options stack top to bottom, a marker shows the active one. */
class ImageSelector final : public juce::Component
{
public:
    ImageSelector()
    {
        setOpaque(false);
        setWantsKeyboardFocus(false);
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void setOptionCount(int count) noexcept { optionCount_ = juce::jmax(2, count); }
    void setMarkerColour(juce::Colour colour) noexcept { markerColour_ = colour; }

    void setSelectedIndex(int index, juce::NotificationType notification = juce::sendNotification)
    {
        index = juce::jlimit(0, optionCount_ - 1, index);
        if (index == selected_)
            return;
        selected_ = index;
        repaint();
        if (notification != juce::dontSendNotification && onChange)
            onChange(selected_);
    }

    [[nodiscard]] int getSelectedIndex() const noexcept { return selected_; }

    std::function<void(int)> onChange;

    void mouseDown(const juce::MouseEvent& e) override
    {
        const float rowH = static_cast<float>(getHeight()) / static_cast<float>(optionCount_);
        setSelectedIndex(static_cast<int>(e.position.y / juce::jmax(1.f, rowH)));
    }

    // artArea is normalised to this component's bounds.
    void setOptionArt(juce::Array<juce::Image> images, juce::Rectangle<float> artArea)
    {
        optionArt_ = std::move(images);
        artArea_ = artArea;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        if (juce::isPositiveAndBelow(selected_, optionArt_.size()) && optionArt_[selected_].isValid())
        {
            const auto bounds = getLocalBounds().toFloat();
            const juce::Rectangle<float> dest(bounds.getX() + artArea_.getX() * bounds.getWidth(),
                                              bounds.getY() + artArea_.getY() * bounds.getHeight(),
                                              artArea_.getWidth() * bounds.getWidth(),
                                              artArea_.getHeight() * bounds.getHeight());
            g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
            g.drawImage(optionArt_[selected_], dest, juce::RectanglePlacement::stretchToFit);
            return;
        }

        const float rowH = static_cast<float>(getHeight()) / static_cast<float>(optionCount_);
        const float r = juce::jmax(2.f, rowH * 0.18f);
        const float cy = rowH * (static_cast<float>(selected_) + 0.5f);
        g.setColour(markerColour_);
        g.fillEllipse(r * 0.5f, cy - r, r * 2.f, r * 2.f);
    }

private:
    int optionCount_ = 3;
    int selected_ = 0;
    juce::Colour markerColour_ { 0xffe8d34a };
    juce::Array<juce::Image> optionArt_;
    juce::Rectangle<float> artArea_ { 0.f, 0.f, 1.f, 1.f };
};

[[nodiscard]] float varNum(const juce::var& obj, const char* key, float fallback = 0.f)
{
    if (auto* d = obj.getDynamicObject())
    {
        const auto value = d->getProperty(key);
        if (value.isInt() || value.isInt64() || value.isDouble())
            return static_cast<float>(static_cast<double>(value));
    }
    return fallback;
}

[[nodiscard]] juce::String varStr(const juce::var& obj, const char* key)
{
    if (auto* d = obj.getDynamicObject())
        return d->getProperty(key).toString();
    return {};
}

[[nodiscard]] bool varBool(const juce::var& obj, const char* key, bool fallback = false)
{
    if (auto* d = obj.getDynamicObject())
    {
        const auto value = d->getProperty(key);
        if (value.isBool() || value.isInt() || value.isInt64())
            return static_cast<bool>(value);
    }
    return fallback;
}

[[nodiscard]] juce::File bundleDirFromIdentity(const juce::File& identity)
{
    if (identity.isDirectory() && identity.getFileName().endsWithIgnoreCase(".kbplug"))
        return identity;
    if (identity.existsAsFile())
    {
        const auto parent = identity.getParentDirectory();
        if (parent.getFileName().endsWithIgnoreCase(".kbplug"))
            return parent;
        return parent;
    }
    return identity;
}

[[nodiscard]] juce::File gfxPacksRoot()
{
    if (const char* env = std::getenv("GFX_PACKS_ROOT"); env != nullptr && env[0] != '\0')
    {
        const juce::File fromEnv(env);
        if (fromEnv.isDirectory())
            return fromEnv;
    }
#ifdef GFX_PACKS_ROOT
    {
        const juce::File fromDefine(GFX_PACKS_ROOT);
        if (fromDefine.isDirectory())
            return fromDefine;
    }
#endif
    if (const char* home = std::getenv("HOME"); home != nullptr && home[0] != '\0')
    {
        const juce::File fromHome = juce::File(home).getChildFile("atomtheme-gfx-packs");
        if (fromHome.isDirectory())
            return fromHome;
    }
    return {};
}

struct ArtManifest
{
    juce::Array<juce::File> decode;
    juce::StringArray knockout;
    juce::File faceBack;
    juce::File faceStomp;
    juce::Rectangle<float> faceStompArea;
};

[[nodiscard]] ArtManifest collectArtManifest(const juce::File& profileDir, const juce::var& panel)
{
    ArtManifest manifest;
    juce::StringArray unique;
    auto add = [&](const juce::String& relative, bool knockout)
    {
        if (relative.isEmpty())
            return juce::File();
        const auto file = profileDir.getChildFile(relative);
        if (! file.existsAsFile())
            return juce::File();
        unique.addIfNotAlreadyThere(file.getFullPathName());
        if (knockout)
            manifest.knockout.addIfNotAlreadyThere(file.getFullPathName());
        return file;
    };

    if (auto* obj = panel.getDynamicObject())
        manifest.faceBack = add(obj->getProperty("background").toString(), true);

    if (auto* list = panel.getProperty("controls", juce::var()).getArray())
    {
        for (const auto& control : *list)
        {
            add(varStr(control, "sprite"), false);
            const auto art = add(varStr(control, "art"), true);
            add(varStr(control, "art_pressed"), true);
            add(varStr(control, "art_off"), false);
            add(varStr(control, "art_on"), false);
            if (auto* options = control.getProperty("option_art", juce::var()).getArray())
                for (const auto& option : *options)
                    add(option.toString(), false);

            if (manifest.faceStomp.getFullPathName().isEmpty()
                && varStr(control, "art_off").isEmpty()
                && art.existsAsFile())
            {
                float w = varNum(control, "w");
                float h = varNum(control, "h");
                if (w <= 0.f)
                    w = 360.f;
                if (h <= 0.f)
                    h = 450.f;
                manifest.faceStomp = art;
                manifest.faceStompArea = { varNum(control, "x"), varNum(control, "y"), w, h };
            }
        }
    }

    for (const auto& path : unique)
        manifest.decode.add(juce::File(path));
    return manifest;
}

juce::Image knockOutNearWhite(juce::Image image, int threshold = 248)
{
    if (! image.isValid())
        return image;
    if (image.getFormat() != juce::Image::ARGB)
        image = image.convertedToFormat(juce::Image::ARGB);
    else
        image = image.createCopy();

    const auto cutoff = static_cast<juce::uint8>(juce::jlimit(0, 255, threshold));
    juce::Image::BitmapData pixels(image, juce::Image::BitmapData::readWrite);
    for (int y = 0; y < image.getHeight(); ++y)
    {
        for (int x = 0; x < image.getWidth(); ++x)
        {
            auto* pixel = pixels.getPixelPointer(x, y);
            if (pixel[juce::PixelARGB::indexR] >= cutoff
                && pixel[juce::PixelARGB::indexG] >= cutoff
                && pixel[juce::PixelARGB::indexB] >= cutoff)
            {
                pixel[juce::PixelARGB::indexA] = 0;
            }
        }
    }
    return image;
}

juce::Image compositeOntoCopy(const juce::Image& destIn, const juce::Image& src, juce::Rectangle<float> area)
{
    if (! destIn.isValid())
        return {};
    auto dest = destIn.createCopy();
    if (dest.getFormat() != juce::Image::ARGB)
        dest = dest.convertedToFormat(juce::Image::ARGB);
    if (src.isValid() && ! area.isEmpty())
    {
        juce::Graphics g(dest);
        g.drawImage(src, area);
    }
    return dest;
}

juce::Rectangle<int> opaqueBounds(const juce::Image& image)
{
    if (! image.isValid())
        return {};

    int minX = image.getWidth();
    int minY = image.getHeight();
    int maxX = -1;
    int maxY = -1;
    juce::Image::BitmapData pixels(image, juce::Image::BitmapData::readOnly);
    for (int y = 0; y < image.getHeight(); ++y)
    {
        for (int x = 0; x < image.getWidth(); ++x)
        {
            if (pixels.getPixelPointer(x, y)[juce::PixelARGB::indexA] > 12)
            {
                minX = juce::jmin(minX, x);
                minY = juce::jmin(minY, y);
                maxX = juce::jmax(maxX, x);
                maxY = juce::jmax(maxY, y);
            }
        }
    }
    if (maxX < minX)
        return image.getBounds();
    return { minX, minY, maxX - minX + 1, maxY - minY + 1 };
}

class GfxArtCache
{
public:
    static GfxArtCache& instance()
    {
        static GfxArtCache cache;
        return cache;
    }

    [[nodiscard]] static juce::String rawKey(const juce::File& file)
    {
        return file.getFileName()
               + "|" + juce::String(file.getSize())
               + "|" + juce::String(file.getLastModificationTime().toMilliseconds() / 1000);
    }

    [[nodiscard]] static juce::String knockoutKey(const juce::File& file)
    {
        return rawKey(file) + "|ko";
    }

    [[nodiscard]] static bool onMessageThread()
    {
        if (auto* mm = juce::MessageManager::getInstanceWithoutCreating())
            return mm->isThisTheMessageThread();
        return false;
    }

    [[nodiscard]] bool containsKey(const juce::String& key) const
    {
        const juce::ScopedLock sl(lock_);
        return images_.find(key) != images_.end();
    }

    [[nodiscard]] static juce::String faceKey(const ArtManifest& manifest)
    {
        if (! manifest.faceBack.existsAsFile())
            return {};
        auto key = knockoutKey(manifest.faceBack) + "|face";
        if (manifest.faceStomp.existsAsFile())
        {
            key += "|" + knockoutKey(manifest.faceStomp)
                   + "|" + juce::String(manifest.faceStompArea.getX(), 2)
                   + "," + juce::String(manifest.faceStompArea.getY(), 2)
                   + "," + juce::String(manifest.faceStompArea.getWidth(), 2)
                   + "," + juce::String(manifest.faceStompArea.getHeight(), 2);
        }
        return key;
    }

    [[nodiscard]] bool hasValid(const juce::String& key) const
    {
        const juce::ScopedLock sl(lock_);
        if (auto it = images_.find(key); it != images_.end())
            return it->second.isValid();
        return false;
    }

    [[nodiscard]] bool isReady(const ArtManifest& manifest) const
    {
        for (const auto& file : manifest.decode)
            if (! hasValid(rawKey(file)))
                return false;
        for (const auto& path : manifest.knockout)
            if (! hasValid(knockoutKey(juce::File(path))))
                return false;
        if (const auto key = faceKey(manifest); key.isNotEmpty() && ! hasValid(key))
            return false;
        return true;
    }

    juce::Image lookup(const juce::String& key) const
    {
        const juce::ScopedLock sl(lock_);
        if (auto it = images_.find(key); it != images_.end())
            return it->second;
        return {};
    }

    juce::Image getOrLoad(const juce::String& key, const std::function<juce::Image()>& loader)
    {
        if (auto cached = lookup(key); cached.isValid())
            return cached;

        // Decode on a worker. A UI-thread stall is a freeze, not the
        // variable-height split seen when switching models.
        if (onMessageThread())
            return {};

        std::shared_ptr<juce::WaitableEvent> waiter;
        {
            const juce::ScopedLock sl(lock_);
            if (auto it = images_.find(key); it != images_.end())
                return it->second;

            auto& slot = inflight_[key];
            if (slot != nullptr)
                waiter = slot;
            else
                slot = std::make_shared<juce::WaitableEvent>(true);
        }

        if (waiter != nullptr)
        {
            waiter->wait();
            return lookup(key);
        }

        auto image = loader();
        {
            const juce::ScopedLock sl(lock_);
            images_[key] = image;
            if (auto it = inflight_.find(key); it != inflight_.end())
            {
                it->second->signal();
                inflight_.erase(it);
            }
        }
        return image;
    }

    juce::Image get(const juce::File& file)
    {
        return getOrLoad(rawKey(file), [this, file] { return decodeFile(file); });
    }

    juce::Image getKnockedOut(const juce::File& file)
    {
        return getOrLoad(knockoutKey(file), [this, file]
        {
            return knockOutNearWhite(get(file));
        });
    }

    juce::Image renderFace(const ArtManifest& manifest)
    {
        if (! manifest.faceBack.existsAsFile())
            return {};
        auto dest = getKnockedOut(manifest.faceBack);
        if (! dest.isValid())
            dest = get(manifest.faceBack);
        if (! dest.isValid())
            return {};
        // Stomp frames are painted by ImagePushButton (normal vs pressed).
        // Baking the rest frame into the face would stack under the pressed
        // PNG and darken its shadow.
        return compositeOntoCopy(dest, {}, {});
    }

    juce::Image getFace(const ArtManifest& manifest)
    {
        const auto key = faceKey(manifest);
        if (key.isEmpty())
            return {};
        if (onMessageThread())
            return lookup(key);
        return getOrLoad(key, [this, manifest] { return renderFace(manifest); });
    }

    void prefetchAsync(const ArtManifest& manifest, std::function<void()> onDone)
    {
        auto files = manifest.decode;
        auto done = std::make_shared<std::function<void()>>(std::move(onDone));
        auto finish = [this, manifest, done]
        {
            getFace(manifest);
            if (*done)
                juce::MessageManager::callAsync(*done);
        };

        if (files.isEmpty())
        {
            juce::Thread::launch([finish] { finish(); });
            return;
        }

        auto remaining = std::make_shared<std::atomic<int>>(files.size());
        auto knockout = manifest.knockout;
        for (const auto& file : files)
        {
            juce::Thread::launch([this, file, knockout, remaining, finish]
            {
                get(file);
                if (knockout.contains(file.getFullPathName()))
                    getKnockedOut(file);
                if (remaining->fetch_sub(1) == 1)
                    finish();
            });
        }
    }

private:
    [[nodiscard]] static juce::File cacheFileFor(const juce::File& source)
    {
        const auto name = source.getFileNameWithoutExtension()
                          + "_" + juce::String(source.getSize())
                          + "_" + juce::String(source.getLastModificationTime().toMilliseconds() / 1000)
                          + ".argb";
        return juce::File::getSpecialLocation(juce::File::userHomeDirectory)
            .getChildFile(".cache")
            .getChildFile("atomtheme-gfx-art")
            .getChildFile(name);
    }

    static juce::Image readDiskCache(const juce::File& cacheFile)
    {
        if (! cacheFile.existsAsFile())
            return {};

        juce::MemoryBlock block;
        if (! cacheFile.loadFileAsData(block) || block.getSize() < 16)
            return {};

        juce::MemoryInputStream in(block, false);
        if (in.readInt() != 0x31475241) // 'ARG1'
            return {};

        const int w = in.readInt();
        const int h = in.readInt();
        const int packedStride = in.readInt();
        if (w <= 0 || h <= 0 || packedStride != w * 4)
            return {};
        if (in.getNumBytesRemaining() < static_cast<juce::int64>(h) * packedStride)
            return {};

        juce::Image image(juce::Image::ARGB, w, h, false);
        juce::Image::BitmapData dest(image, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < h; ++y)
        {
            auto* line = dest.getLinePointer(y);
            if (dest.pixelStride == 4 && dest.lineStride == packedStride
                && juce::PixelARGB::indexB == 0 && juce::PixelARGB::indexG == 1
                && juce::PixelARGB::indexR == 2 && juce::PixelARGB::indexA == 3)
            {
                in.read(line, packedStride);
            }
            else
            {
                juce::HeapBlock<juce::uint8> row(static_cast<size_t>(packedStride));
                in.read(row.getData(), static_cast<size_t>(packedStride));
                for (int x = 0; x < w; ++x)
                {
                    const auto* src = row.getData() + x * 4;
                    auto* pixel = dest.getPixelPointer(x, y);
                    pixel[juce::PixelARGB::indexB] = src[0];
                    pixel[juce::PixelARGB::indexG] = src[1];
                    pixel[juce::PixelARGB::indexR] = src[2];
                    pixel[juce::PixelARGB::indexA] = src[3];
                }
            }
        }
        return image;
    }

    static void writeDiskCache(const juce::File& cacheFile, const juce::Image& image)
    {
        if (! image.isValid())
            return;

        juce::Image::BitmapData src(image, juce::Image::BitmapData::readOnly);
        const int w = image.getWidth();
        const int h = image.getHeight();
        const int packedStride = w * 4;
        juce::MemoryOutputStream out;
        out.writeInt(0x31475241);
        out.writeInt(w);
        out.writeInt(h);
        out.writeInt(packedStride);

        juce::HeapBlock<juce::uint8> row(static_cast<size_t>(packedStride));
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const auto* pixel = src.getPixelPointer(x, y);
                auto* dest = row.getData() + x * 4;
                dest[0] = pixel[juce::PixelARGB::indexB];
                dest[1] = pixel[juce::PixelARGB::indexG];
                dest[2] = pixel[juce::PixelARGB::indexR];
                dest[3] = pixel[juce::PixelARGB::indexA];
            }
            out.write(row.getData(), static_cast<size_t>(packedStride));
        }

        cacheFile.getParentDirectory().createDirectory();
        cacheFile.replaceWithData(out.getData(), out.getDataSize());
    }

    static juce::Image decodeFile(const juce::File& file)
    {
        if (! file.existsAsFile())
            return {};

        const auto cacheFile = cacheFileFor(file);
        if (auto cached = readDiskCache(cacheFile); cached.isValid())
            return cached;

        auto image = juce::ImageFileFormat::loadFrom(file);
        if (image.isValid())
        {
            juce::Thread::launch([cacheFile, image] { writeDiskCache(cacheFile, image); });
        }
        return image;
    }

    mutable juce::CriticalSection lock_;
    std::map<juce::String, juce::Image> images_;
    std::map<juce::String, std::shared_ptr<juce::WaitableEvent>> inflight_;
};
}  // namespace

juce::File resolveGfxPackDirectory(const AudioEffectFrameworkProcessor& processor)
{
    juce::File bundle;
    juce::String gfxKey;

    const auto& engine = processor.getKbussEngine();
    if (const auto path = engine.dynamicMiddlePluginBundlePath())
        bundle = bundleDirFromIdentity(juce::File(juce::String(*path)));
    gfxKey = juce::String(engine.dynamicMiddleGfxKey());

    if (gfxKey.isEmpty())
    {
        if (auto* middle = engine.getMiddleProcessor())
        {
            const auto& desc = middle->description();
            gfxKey = desc.gfx_key.empty() ? juce::String(desc.composite_key)
                                          : juce::String(desc.gfx_key);
        }
    }

    if (bundle.isDirectory())
    {
        const auto bundled = bundle.getChildFile("gfx");
        if (bundled.getChildFile("pack.yaml").existsAsFile())
            return bundled;
    }

    const auto root = gfxPacksRoot();
    if (root.isDirectory() && gfxKey.isNotEmpty())
    {
        const auto pack = root.getChildFile("packs").getChildFile(gfxKey);
        if (pack.getChildFile("pack.yaml").existsAsFile())
            return pack;
    }

    if (gfxKey.isNotEmpty())
    {
        const auto viaKbuss = kbuss::resolve_gfx_pack_dir(
            bundle.getFullPathName().toStdString(),
            gfxKey.toStdString(),
            gfxPacksRoot().getFullPathName().toStdString());
        const juce::File resolved(viaKbuss.string());
        if (resolved.getChildFile("pack.yaml").existsAsFile())
            return resolved;
    }

    return {};
}

void prefetchGfxPackArt(const AudioEffectFrameworkProcessor& processor, const juce::String& profile)
{
    const auto packDir = resolveGfxPackDirectory(processor);
    if (! packDir.isDirectory())
        return;

    const auto profileDir = packDir.getChildFile(profile);
    const auto panelFile = profileDir.getChildFile("panel.json");
    if (! panelFile.existsAsFile())
        return;

    const auto parsed = juce::JSON::parse(panelFile.loadFileAsString());
    if (! parsed.isObject())
        return;

    GfxArtCache::instance().prefetchAsync(collectArtManifest(profileDir, parsed), {});
}

GfxPedalPanel::GfxPedalPanel(AudioEffectFrameworkProcessor& processor, AtomLookAndFeel& lookAndFeel)
    : processor_(processor), atomLookAndFeel_(lookAndFeel)
{
    setOpaque(true);
    setBufferedToImage(true);
    setVisible(false);
}

GfxPedalPanel::~GfxPedalPanel()
{
    ++loadGeneration_;
}

void GfxPedalPanel::clearControls()
{
    layout_.clear();
    widgets_.clear();
    removeAllChildren();
    bypassLight_ = nullptr;
    background_ = {};
    presentation_ = {};
    boundKnobs_.clear();
    boundSwitches_.clear();
    faceReady_ = false;
    pendingProfileDir_ = juce::File();
    pendingPanel_ = juce::var();
}

juce::Image GfxPedalPanel::loadArt(const juce::File& profileDir, const juce::String& relative) const
{
    if (relative.isEmpty())
        return {};
    const auto file = profileDir.getChildFile(relative);
    if (! file.existsAsFile())
        return {};
    return GfxArtCache::instance().get(file);
}

juce::Image GfxPedalPanel::loadKnockedOutArt(const juce::File& profileDir, const juce::String& relative) const
{
    if (relative.isEmpty())
        return {};
    const auto file = profileDir.getChildFile(relative);
    if (! file.existsAsFile())
        return {};
    return GfxArtCache::instance().getKnockedOut(file);
}

void GfxPedalPanel::refreshFromProcessor()
{
    for (auto& binding : boundKnobs_)
    {
        auto* knob = dynamic_cast<SpriteKnob*>(binding.second);
        if (knob == nullptr)
            continue;
        float initial = processor_.getMiddleParamDomain(binding.first, 0.5f);
        if (auto* desc = processor_.getKbussEngine().paramDescriptor(
                processor_.getKbussEngine().middleProcessorId(), binding.first.toStdString()))
        {
            initial = processor_.getMiddleParamDomain(binding.first, desc->default_domain);
        }
        knob->setValue(initial, juce::dontSendNotification);
    }
    for (auto& binding : boundSwitches_)
    {
        const float value = processor_.getMiddleParamDomain(binding.first, 0.f);
        if (auto* toggle = dynamic_cast<ImageToggle*>(binding.second))
            toggle->setToggleState(value >= 0.5f, juce::dontSendNotification);
        else if (auto* selector = dynamic_cast<ImageSelector*>(binding.second))
            selector->setSelectedIndex(juce::roundToInt(value), juce::dontSendNotification);
    }
    syncBypassIndicator();
}

bool GfxPedalPanel::isLoadedFrom(const juce::File& packDir) const noexcept
{
    return loadedPackDir_.getFullPathName() == packDir.getFullPathName();
}

void GfxPedalPanel::finishPendingLoad()
{
    if (! pendingPanel_.isObject() || ! pendingProfileDir_.isDirectory())
        return;

    const auto manifest = collectArtManifest(pendingProfileDir_, pendingPanel_);
    if (! GfxArtCache::instance().isReady(manifest))
    {
        const auto gen = loadGeneration_;
        GfxArtCache::instance().prefetchAsync(manifest, [safe = juce::Component::SafePointer<GfxPedalPanel>(this), gen]()
        {
            if (safe == nullptr || safe->loadGeneration_ != gen)
                return;
            safe->finishPendingLoad();
        });
        return;
    }

    const auto profileDir = pendingProfileDir_;
    const auto panel = pendingPanel_;
    if (! loadPanel(profileDir, panel))
    {
        const auto gen = loadGeneration_;
        GfxArtCache::instance().prefetchAsync(manifest, [safe = juce::Component::SafePointer<GfxPedalPanel>(this), gen]()
        {
            if (safe == nullptr || safe->loadGeneration_ != gen)
                return;
            safe->finishPendingLoad();
        });
        return;
    }

    pendingProfileDir_ = juce::File();
    pendingPanel_ = juce::var();
    // Resize the host to this face first, then show. Revealing at the previous
    // model's height is what made the pedal appear in two bands.
    setVisible(false);
    notifyPreferredSizeChanged();
    resized();
    setVisible(true);
    repaint();
}

bool GfxPedalPanel::load(const juce::File& packDir, const juce::String& profile)
{
    ++loadGeneration_;
    clearControls();
    loadedPackDir_ = juce::File();
    if (! packDir.isDirectory())
        return false;

    const auto profileDir = packDir.getChildFile(profile);
    const auto panelFile = profileDir.getChildFile("panel.json");
    if (! panelFile.existsAsFile())
        return false;

    const auto parsed = juce::JSON::parse(panelFile.loadFileAsString());
    if (! parsed.isObject())
        return false;

    if (auto* obj = parsed.getDynamicObject())
    {
        const auto size = obj->getProperty("logical_size");
        if (auto* arr = size.getArray(); arr != nullptr && arr->size() >= 2)
        {
            logicalW_ = static_cast<float>(static_cast<double>((*arr)[0]));
            logicalH_ = static_cast<float>(static_cast<double>((*arr)[1]));
        }
    }

    if (logicalW_ <= 1.f || logicalH_ <= 1.f)
        return false;

    loadedPackDir_ = packDir;
    setVisible(false);

    const auto manifest = collectArtManifest(profileDir, parsed);
    if (GfxArtCache::instance().isReady(manifest))
        return loadPanel(profileDir, parsed);

    pendingProfileDir_ = profileDir;
    pendingPanel_ = parsed;
    const auto gen = loadGeneration_;
    GfxArtCache::instance().prefetchAsync(manifest, [safe = juce::Component::SafePointer<GfxPedalPanel>(this), gen]()
    {
        if (safe == nullptr || safe->loadGeneration_ != gen)
            return;
        safe->finishPendingLoad();
    });
    return true;
}

bool GfxPedalPanel::loadPanel(const juce::File& profileDir, const juce::var& panel)
{
    widgets_.clear();
    layout_.clear();
    boundKnobs_.clear();
    boundSwitches_.clear();
    removeAllChildren();
    bypassLight_ = nullptr;
    faceReady_ = false;

    const auto manifest = collectArtManifest(profileDir, panel);
    if (auto* obj = panel.getDynamicObject())
    {
        const auto size = obj->getProperty("logical_size");
        if (auto* arr = size.getArray(); arr != nullptr && arr->size() >= 2)
        {
            logicalW_ = static_cast<float>(static_cast<double>((*arr)[0]));
            logicalH_ = static_cast<float>(static_cast<double>((*arr)[1]));
        }
    }

    background_ = GfxArtCache::instance().getFace(manifest);
    if (! background_.isValid())
        return false;

    if (logicalW_ <= 1.f || logicalH_ <= 1.f)
        return false;

    preferredWidth_ = juce::roundToInt(logicalW_);
    preferredHeight_ = juce::roundToInt(logicalH_);

    const auto controls = panel.getProperty("controls", juce::var());
    auto* list = controls.getArray();
    if (list == nullptr)
        return false;

    bool addedGraphic = false;
    for (const auto& control : *list)
    {
        const auto type = varStr(control, "type");
        const auto paramId = varStr(control, "param_id");
        const float x = varNum(control, "x");
        const float y = varNum(control, "y");
        const float r = varNum(control, "r");
        float w = varNum(control, "w");
        float h = varNum(control, "h");
        if (w <= 0.f && r > 0.f)
            w = r * 2.f;
        if (h <= 0.f && r > 0.f)
            h = r * 2.f;

        if (type == "knob")
        {
            const auto spritePath = varStr(control, "sprite");
            auto sheet = loadArt(profileDir, spritePath);
            if (spritePath.isNotEmpty() && ! sheet.isValid())
                return false;
            if (! sheet.isValid())
                continue;

            auto knob = std::make_unique<SpriteKnob>();
            const int cols = juce::jmax(1, juce::roundToInt(varNum(control, "sprite_cols", 12.f)));
            const int frames = juce::roundToInt(varNum(control, "sprite_frames"));
            knob->setSpriteSheet(sheet, cols, frames);
            knob->setAngleRange(varNum(control, "angle_start", 30.f),
                                varNum(control, "angle_end", 330.f));

            float minDomain = 0.f;
            float maxDomain = 1.f;
            float initial = 0.5f;
            if (auto* desc = processor_.getKbussEngine().paramDescriptor(
                    processor_.getKbussEngine().middleProcessorId(), paramId.toStdString()))
            {
                minDomain = desc->min_domain;
                maxDomain = desc->max_domain;
                initial = processor_.getMiddleParamDomain(paramId, desc->default_domain);
            }
            else
            {
                initial = processor_.getMiddleParamDomain(paramId, 0.5f);
            }

            knob->setRange(minDomain, maxDomain, (maxDomain - minDomain) <= 1.0f ? 0.01 : 1.0);
            knob->setDefaultValue(initial);
            knob->setValue(initial, juce::dontSendNotification);
            knob->onValueChanged = [this, paramId](double v) {
                processor_.setMiddleParamDomain(paramId, static_cast<float>(v));
            };

            auto* raw = knob.get();
            addAndMakeVisible(raw);
            widgets_.add(knob.release());
            boundKnobs_.push_back({ paramId, raw });
            layout_.push_back({ raw, x, y, w > 0.f ? w : 64.f, h > 0.f ? h : 64.f });
            addedGraphic = true;
            continue;
        }

        if (type == "toggle" && paramId == "bypass")
        {
            const auto artOffPath = varStr(control, "art_off");
            const auto artOnPath = varStr(control, "art_on");
            const auto artOff = loadArt(profileDir, artOffPath);
            const auto artOn = loadArt(profileDir, artOnPath);
            const auto art = loadKnockedOutArt(profileDir, varStr(control, "art"));
            const auto artPressed = loadKnockedOutArt(profileDir, varStr(control, "art_pressed"));

            if (artOffPath.isNotEmpty() || artOnPath.isNotEmpty())
            {
                if (! artOff.isValid() && ! artOn.isValid())
                    return false;
            }

            if (artOff.isValid() || artOn.isValid())
            {
                auto light = std::make_unique<ImageToggle>();
                light->setImages(artOff, artOn);
                light->setClickingTogglesState(true);
                light->setToggleState(! hostBypassed(), juce::dontSendNotification);
                light->onClick = [this, raw = light.get()]() {
                    // ToggleButton has already flipped: ON = effect engaged = not bypassed.
                    setHostBypassed(! raw->getToggleState());
                };
                auto* raw = light.get();
                addAndMakeVisible(raw);
                widgets_.add(light.release());
                layout_.push_back({ raw, x, y, w > 0.f ? w : 52.f, h > 0.f ? h : 52.f });
                bypassLight_ = raw;
                addedGraphic = true;
                continue;
            }

            const bool expectsStomp = varStr(control, "art").isNotEmpty()
                                      || varStr(control, "art_pressed").isNotEmpty();
            if (expectsStomp && ! art.isValid() && ! artPressed.isValid())
                return false;

            if (art.isValid() || artPressed.isValid())
            {
                auto stomp = std::make_unique<ImagePushButton>();
                stomp->setImages(art, artPressed.isValid() ? artPressed : art);
                stomp->onClick = [this]() {
                    setHostBypassed(! hostBypassed());
                    syncBypassIndicator();
                };
                auto* raw = stomp.get();
                addAndMakeVisible(raw);
                widgets_.add(stomp.release());
                layout_.push_back({ raw, x, y, w > 0.f ? w : 360.f, h > 0.f ? h : 450.f });
                addedGraphic = true;
                continue;
            }
        }

        if (type == "toggle" && paramId.isNotEmpty())
        {
            const auto artOff = loadArt(profileDir, varStr(control, "art_off"));
            const auto artOn = loadArt(profileDir, varStr(control, "art_on"));
            if (! artOff.isValid() && ! artOn.isValid())
                return false;

            auto toggle = std::make_unique<ImageToggle>();
            toggle->setImages(artOff, artOn);
            toggle->setClickingTogglesState(true);
            toggle->setToggleState(processor_.getMiddleParamDomain(paramId, 0.f) >= 0.5f,
                                   juce::dontSendNotification);
            toggle->onClick = [this, paramId, raw = toggle.get()]() {
                processor_.setMiddleParamDomain(paramId, raw->getToggleState() ? 1.0f : 0.0f);
            };
            auto* raw = toggle.get();
            addAndMakeVisible(raw);
            widgets_.add(toggle.release());
            boundSwitches_.push_back({ paramId, raw });
            layout_.push_back({ raw, x, y, w > 0.f ? w : 12.f, h > 0.f ? h : 25.f });
            addedGraphic = true;
            continue;
        }

        if (type == "selector" && paramId.isNotEmpty() && w > 0.f && h > 0.f)
        {
            auto selector = std::make_unique<ImageSelector>();
            selector->setOptionCount(juce::roundToInt(varNum(control, "options", 3.f)));
            if (const auto colour = varStr(control, "marker_colour"); colour.isNotEmpty())
                selector->setMarkerColour(juce::Colour::fromString(colour));
            if (auto* options = control.getProperty("option_art", juce::var()).getArray())
            {
                juce::Array<juce::Image> images;
                for (const auto& option : *options)
                    images.add(loadArt(profileDir, option.toString()));
                const auto* rect = control.getProperty("art_rect", juce::var()).getArray();
                juce::Rectangle<float> area { 0.f, 0.f, 1.f, 1.f };
                if (rect != nullptr && rect->size() == 4)
                    area = { (static_cast<float>((*rect)[0]) - x) / w, (static_cast<float>((*rect)[1]) - y) / h,
                             static_cast<float>((*rect)[2]) / w, static_cast<float>((*rect)[3]) / h };
                selector->setOptionArt(std::move(images), area);
            }
            selector->setSelectedIndex(juce::roundToInt(processor_.getMiddleParamDomain(paramId, 0.f)),
                                       juce::dontSendNotification);
            selector->onChange = [this, paramId](int index) {
                processor_.setMiddleParamDomain(paramId, static_cast<float>(index));
            };
            auto* raw = selector.get();
            addAndMakeVisible(raw);
            widgets_.add(selector.release());
            boundSwitches_.push_back({ paramId, raw });
            layout_.push_back({ raw, x, y, w, h });
            addedGraphic = true;
            continue;
        }

        // Graphic packs only show artwork controls. White-box extras (quality, clipper,
        // other enums) stay off the pedal face.
    }

    if (addedGraphic)
        fitToArtwork();

    flattenOpaqueFace();
    rebuildPresentation();
    faceReady_ = addedGraphic;
    return addedGraphic;
}

void GfxPedalPanel::notifyPreferredSizeChanged()
{
    if (onPreferredSizeChanged)
        onPreferredSizeChanged();
}

juce::Rectangle<float> GfxPedalPanel::fittedArtBounds() const noexcept
{
    if (logicalW_ <= 0.f || logicalH_ <= 0.f)
        return getLocalBounds().toFloat();

    const float scale = juce::jmin(static_cast<float>(getWidth()) / logicalW_,
                                   static_cast<float>(getHeight()) / logicalH_);
    const float w = logicalW_ * scale;
    const float h = logicalH_ * scale;
    return { (static_cast<float>(getWidth()) - w) * 0.5f,
             (static_cast<float>(getHeight()) - h) * 0.5f,
             w,
             h };
}

void GfxPedalPanel::fitToArtwork()
{
    juce::Rectangle<int> box;
    if (background_.isValid())
        box = opaqueBounds(background_);

    for (const auto& spec : layout_)
    {
        box = box.getUnion(juce::Rectangle<int>(juce::roundToInt(spec.x),
                                                juce::roundToInt(spec.y),
                                                juce::jmax(1, juce::roundToInt(spec.w)),
                                                juce::jmax(1, juce::roundToInt(spec.h))));
    }

    if (box.isEmpty())
        return;

    const auto canvas = juce::Rectangle<int>(0, 0, juce::roundToInt(logicalW_), juce::roundToInt(logicalH_));
    box = box.expanded(10).getIntersection(canvas);
    if (box.isEmpty() || box == canvas)
        return;

    if (background_.isValid())
        background_ = background_.getClippedImage(box);

    for (auto& spec : layout_)
    {
        spec.x -= static_cast<float>(box.getX());
        spec.y -= static_cast<float>(box.getY());
    }

    logicalW_ = static_cast<float>(box.getWidth());
    logicalH_ = static_cast<float>(box.getHeight());
    preferredWidth_ = box.getWidth();
    preferredHeight_ = box.getHeight();
}

void GfxPedalPanel::flattenOpaqueFace()
{
    // Opaque RGB so a window-resize expose blits one buffer instead of
    // compositing a tall transparent face into the leftover upper band.
    if (! background_.isValid())
        return;

    const auto fill = atomLookAndFeel_.findColour(juce::ResizableWindow::backgroundColourId);
    juce::Image flat(juce::Image::RGB, background_.getWidth(), background_.getHeight(), false);
    {
        juce::Graphics g(flat);
        g.fillAll(fill);
        g.drawImageAt(background_, 0, 0);
    }
    background_ = std::move(flat);
}

void GfxPedalPanel::rebuildPresentation()
{
    if (! background_.isValid() || getWidth() < 2 || getHeight() < 2)
    {
        presentation_ = {};
        return;
    }

    const auto fill = atomLookAndFeel_.findColour(juce::ResizableWindow::backgroundColourId);
    presentation_ = juce::Image(juce::Image::RGB, getWidth(), getHeight(), false);
    juce::Graphics g(presentation_);
    g.fillAll(fill);
    g.drawImage(background_, fittedArtBounds());
}

bool GfxPedalPanel::hostBypassed() const
{
    if (auto* param = processor_.parameters.valueTreeState.getParameter(processor_.paramBypass.paramID))
        return param->getValue() >= 0.5f;
    return false;
}

void GfxPedalPanel::setHostBypassed(bool bypassed)
{
    auto* param = processor_.parameters.valueTreeState.getParameter(processor_.paramBypass.paramID);
    if (param == nullptr)
        return;
    const float next = bypassed ? 1.0f : 0.0f;
    if (std::abs(param->getValue() - next) < 1.0e-4f)
        return;
    param->beginChangeGesture();
    param->setValueNotifyingHost(next);
    param->endChangeGesture();
}

void GfxPedalPanel::syncBypassIndicator()
{
    auto* light = dynamic_cast<ImageToggle*>(bypassLight_);
    if (light == nullptr)
        return;
    light->setToggleState(! hostBypassed(), juce::dontSendNotification);
}

void GfxPedalPanel::paint(juce::Graphics& g)
{
    if (presentation_.isValid())
    {
        g.drawImageAt(presentation_, 0, 0);
        return;
    }
    if (! background_.isValid() || logicalW_ <= 0.f || logicalH_ <= 0.f)
        return;

    g.fillAll(atomLookAndFeel_.findColour(juce::ResizableWindow::backgroundColourId));
    g.drawImage(background_, fittedArtBounds());
}

void GfxPedalPanel::resized()
{
    if (logicalW_ <= 0.f || logicalH_ <= 0.f)
        return;

    const auto dest = fittedArtBounds();
    const float scale = dest.getWidth() / logicalW_;
    for (const auto& spec : layout_)
    {
        if (spec.widget == nullptr)
            continue;
        spec.widget->setBounds(juce::roundToInt(dest.getX() + spec.x * scale),
                               juce::roundToInt(dest.getY() + spec.y * scale),
                               juce::jmax(1, juce::roundToInt(spec.w * scale)),
                               juce::jmax(1, juce::roundToInt(spec.h * scale)));
    }
    rebuildPresentation();
}
