// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "displayedimagestate.h"

#include "imageanimationplayer.h"
#include "imagerendering.h"

#include <QObject>
#include <memory>
#include <utility>

namespace KiriView {
DisplayedImageState::DisplayedImageState(
    QObject *context, ImageChangedCallback imageChanged, AnimationErrorCallback animationError)
    : m_imageChanged(std::move(imageChanged))
    , m_animationError(std::move(animationError))
{
    auto frameReady = [this](const QImage &image) { setImage(image); };
    auto animationErrorCallback
        = [this](const QString &errorString) { m_animationError(errorString); };
    m_animationPlayer = std::make_unique<ImageAnimationPlayer>(
        context, std::move(frameReady), std::move(animationErrorCallback));
}

DisplayedImageState::~DisplayedImageState() = default;

bool DisplayedImageState::hasImage() const
{
    return m_surface != nullptr && !displayedImageSurfaceIsNull(*m_surface);
}

std::shared_ptr<DisplayedImageSurface> DisplayedImageState::imageSurface() const
{
    return m_surface;
}

const QImage &DisplayedImageState::image() const { return m_image; }

QSize DisplayedImageState::imageSize() const
{
    if (m_surface != nullptr) {
        return displayedImageSurfaceSize(*m_surface);
    }

    return {};
}

quint64 DisplayedImageState::revision() const { return m_imageRevision; }

bool DisplayedImageState::isPredecodeCacheable() const { return m_imageIsPredecodeCacheable; }

std::shared_ptr<ImageTileSource> DisplayedImageState::staticImageSource() const
{
    return m_staticImageSource;
}

const QImage &DisplayedImageState::staticImagePreview() const { return m_staticImagePreview; }

const StaticImageDisplayHints &DisplayedImageState::staticImageDisplayHints() const
{
    return m_staticImageDisplayHints;
}

void DisplayedImageState::setPredecodeCacheable(bool cacheable)
{
    m_imageIsPredecodeCacheable = cacheable;
}

void DisplayedImageState::setImage(const QImage &image)
{
    m_image = displayReadyImage(image);
    m_staticImageSource.reset();
    m_staticImagePreview = QImage();
    m_staticImageDisplayHints = StaticImageDisplayHints {};
    m_surface = std::make_shared<DisplayedImageSurface>(LegacyFrameSurface { m_image });
    ++m_imageRevision;
    notifyImageChanged();
}

void DisplayedImageState::setStaticImage(std::shared_ptr<ImageTileSource> source,
    const QImage &preview, StaticImageDisplayHints displayHints, bool useFullImageSurface)
{
    m_image = displayReadyImage(preview);
    m_staticImageSource = std::move(source);
    m_staticImagePreview = m_image;
    m_staticImageDisplayHints = displayHints;
    if (useFullImageSurface) {
        m_surface = std::make_shared<DisplayedImageSurface>(LegacyFrameSurface { m_image });
    } else {
        m_surface = std::make_shared<DisplayedImageSurface>(
            StaticTileSurface { m_staticImageSource, m_image, displayHints });
    }
    ++m_imageRevision;
    notifyImageChanged();
}

bool DisplayedImageState::insertTile(DecodedTile tile)
{
    if (m_surface == nullptr) {
        return false;
    }
    auto *surface = std::get_if<StaticTileSurface>(m_surface.get());
    if (surface == nullptr) {
        return false;
    }

    surface->insertTile(std::move(tile));
    ++m_imageRevision;
    notifyImageChanged();
    return true;
}

void DisplayedImageState::clear()
{
    stopAnimation();
    m_imageIsPredecodeCacheable = false;

    if (m_surface != nullptr || !m_image.isNull()) {
        m_surface.reset();
        m_image = QImage();
        m_staticImageSource.reset();
        m_staticImagePreview = QImage();
        m_staticImageDisplayHints = StaticImageDisplayHints {};
        ++m_imageRevision;
        notifyImageChanged();
    }
}

void DisplayedImageState::startAnimation(
    const QByteArray &data, const QByteArray &format, int loopCount, int firstFrameDelay)
{
    m_animationPlayer->start(data, format, loopCount, firstFrameDelay);
}

void DisplayedImageState::startDecodedAnimation(std::vector<AnimationFrame> frames, int loopCount)
{
    m_animationPlayer->startDecoded(std::move(frames), loopCount);
}

void DisplayedImageState::startHeifSequenceAnimation(const QByteArray &data, int firstFrameDelay)
{
    m_animationPlayer->startHeifSequence(data, firstFrameDelay);
}

void DisplayedImageState::stopAnimation() { m_animationPlayer->stop(); }

void DisplayedImageState::notifyImageChanged()
{
    if (m_imageChanged) {
        m_imageChanged(imageSize());
    }
}
}
