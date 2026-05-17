// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "displayedimagestate.h"

#include "imageanimationplayer.h"
#include "imagecallback.h"
#include "imagerendering.h"

#include <QObject>
#include <memory>
#include <optional>
#include <utility>

namespace KiriView {
DisplayedImageState::DisplayedImageState(
    QObject *context, ImageChangedCallback imageChanged, AnimationErrorCallback animationError)
    : m_imageChanged(std::move(imageChanged))
    , m_animationError(std::move(animationError))
{
    auto frameReady = [this](const QImage &image) { setImage(image); };
    auto animationErrorCallback
        = [this](const QString &errorString) { invokeIfSet(m_animationError, errorString); };
    m_animationPlayer = std::make_unique<ImageAnimationPlayer>(
        context, std::move(frameReady), std::move(animationErrorCallback));
}

DisplayedImageState::~DisplayedImageState() = default;

bool DisplayedImageState::hasImage() const { return m_surface != nullptr && !m_surface->isNull(); }

std::shared_ptr<DisplayedImageSurface> DisplayedImageState::imageSurface() const
{
    return m_surface;
}

const QImage &DisplayedImageState::image() const { return m_image; }

QSize DisplayedImageState::imageSize() const
{
    if (m_surface != nullptr) {
        return m_surface->imageSize();
    }

    return {};
}

quint64 DisplayedImageState::revision() const { return m_imageRevision; }

bool DisplayedImageState::isPredecodeCacheable() const { return m_imageIsPredecodeCacheable; }

std::optional<StaticImagePayload> DisplayedImageState::staticImage() const
{
    if (!m_staticImage.has_value() || !m_staticImage->isValid()) {
        return std::nullopt;
    }

    return m_staticImage;
}

void DisplayedImageState::setPredecodeCacheable(bool cacheable)
{
    m_imageIsPredecodeCacheable = cacheable;
}

void DisplayedImageState::setImage(const QImage &image)
{
    QImage displayImage = displayReadyImage(image);
    replaceDisplayedImage(displayImage,
        std::make_shared<DisplayedImageSurface>(LegacyFrameSurface { displayImage }), std::nullopt);
}

void DisplayedImageState::setStaticImage(StaticImagePayload staticImage, bool useFullImageSurface)
{
    QImage displayImage = displayReadyImage(staticImage.preview);
    staticImage.preview = displayImage;

    std::optional<StaticImagePayload> storedStaticImage(std::move(staticImage));
    std::shared_ptr<DisplayedImageSurface> surface;
    if (useFullImageSurface) {
        surface = std::make_shared<DisplayedImageSurface>(LegacyFrameSurface { displayImage });
    } else {
        surface = std::make_shared<DisplayedImageSurface>(StaticTileSurface { *storedStaticImage });
    }

    replaceDisplayedImage(
        std::move(displayImage), std::move(surface), std::move(storedStaticImage));
}

bool DisplayedImageState::insertTile(DecodedTile tile)
{
    if (m_surface == nullptr) {
        return false;
    }
    auto *surface = m_surface->staticTileSurface();
    if (surface == nullptr) {
        return false;
    }

    if (!surface->insertTile(std::move(tile))) {
        return false;
    }

    finishImageChange();
    return true;
}

void DisplayedImageState::clear()
{
    stopAnimation();
    m_imageIsPredecodeCacheable = false;

    if (m_surface != nullptr || !m_image.isNull()) {
        replaceDisplayedImage(QImage(), nullptr, std::nullopt);
    }
}

void DisplayedImageState::startAnimation(
    const QByteArray &data, const QByteArray &format, int loopCount, int firstFrameDelay)
{
    m_animationPlayer->start(data, format, loopCount, firstFrameDelay);
}

void DisplayedImageState::startApngAnimation(
    const QByteArray &data, int loopCount, int firstFrameDelay)
{
    m_animationPlayer->startApng(data, loopCount, firstFrameDelay);
}

void DisplayedImageState::startDecodedAnimation(std::vector<AnimationFrame> frames, int loopCount)
{
    m_animationPlayer->startDecoded(std::move(frames), loopCount);
}

void DisplayedImageState::startHeifSequenceAnimation(const QByteArray &data)
{
    m_animationPlayer->startHeifSequence(data);
}

void DisplayedImageState::stopAnimation() { m_animationPlayer->stop(); }

void DisplayedImageState::replaceDisplayedImage(QImage image,
    std::shared_ptr<DisplayedImageSurface> surface, std::optional<StaticImagePayload> staticImage)
{
    m_image = std::move(image);
    m_surface = std::move(surface);
    m_staticImage = std::move(staticImage);
    finishImageChange();
}

void DisplayedImageState::finishImageChange()
{
    ++m_imageRevision;
    notifyImageChanged();
}

void DisplayedImageState::notifyImageChanged() { invokeIfSet(m_imageChanged, imageSize()); }
}
