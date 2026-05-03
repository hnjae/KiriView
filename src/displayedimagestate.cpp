// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "displayedimagestate.h"

#include "imageanimationplayer.h"
#include "kiriimagedecoder.h"

#include <QObject>
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

bool DisplayedImageState::hasImage() const { return !m_image.isNull(); }

const QImage &DisplayedImageState::image() const { return m_image; }

QSize DisplayedImageState::imageSize() const
{
    if (m_imageIsSvg) {
        return m_svgIntrinsicSize;
    }

    return m_image.size();
}

quint64 DisplayedImageState::revision() const { return m_imageRevision; }

bool DisplayedImageState::isPredecodeCacheable() const { return m_imageIsPredecodeCacheable; }

void DisplayedImageState::setPredecodeCacheable(bool cacheable)
{
    m_imageIsPredecodeCacheable = cacheable;
}

void DisplayedImageState::setImage(const QImage &image)
{
    clearSvgImage();
    m_image = displayReadyImage(image);
    ++m_imageRevision;
    notifyImageChanged();
}

void DisplayedImageState::setSvgImage(
    QByteArray data, const QSize &intrinsicSize, const QImage &image, const QSize &rasterSize)
{
    m_imageIsSvg = true;
    m_svgData = std::move(data);
    m_svgIntrinsicSize = intrinsicSize;
    m_svgRasterSize = rasterSize;
    m_image = displayReadyImage(image);
    ++m_imageRevision;
    notifyImageChanged();
}

bool DisplayedImageState::updateSvgRaster(
    const QSizeF &displaySize, qreal devicePixelRatio, int maximumTextureSize)
{
    if (!m_imageIsSvg) {
        return true;
    }

    const QSize rasterSize = svgRasterSize(displaySize, devicePixelRatio, maximumTextureSize);
    if (rasterSize.isEmpty()) {
        return false;
    }
    if (!m_image.isNull() && m_svgRasterSize == rasterSize) {
        return true;
    }

    const QImage image = renderSvgImage(m_svgData, rasterSize);
    if (image.isNull()) {
        return false;
    }

    m_image = displayReadyImage(image);
    m_svgRasterSize = rasterSize;
    ++m_imageRevision;
    notifyImageChanged();
    return true;
}

void DisplayedImageState::clear()
{
    stopAnimation();
    m_imageIsPredecodeCacheable = false;
    clearSvgImage();

    if (!m_image.isNull()) {
        m_image = QImage();
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

void DisplayedImageState::clearSvgImage()
{
    m_imageIsSvg = false;
    m_svgData.clear();
    m_svgIntrinsicSize = QSize();
    m_svgRasterSize = QSize();
}

void DisplayedImageState::notifyImageChanged()
{
    if (m_imageChanged) {
        m_imageChanged(imageSize());
    }
}
}
