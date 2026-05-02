// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagezoomstate.h"

#include "imageurl.h"

#include <algorithm>
#include <cmath>

namespace KiriView {
bool imageZoomApproximatelyEqual(qreal left, qreal right) { return std::abs(left - right) < 0.001; }

bool imageZoomApproximatelyEqual(const QSizeF &left, const QSizeF &right)
{
    return imageZoomApproximatelyEqual(left.width(), right.width())
        && imageZoomApproximatelyEqual(left.height(), right.height());
}

const QSize &ImageZoomState::imageSize() const { return m_imageSize; }

const QSizeF &ImageZoomState::viewportSize() const { return m_viewportSize; }

const QSizeF &ImageZoomState::displaySize() const { return m_displaySize; }

qreal ImageZoomState::zoomPercent() const { return m_zoomPercent; }

ImageZoomMode ImageZoomState::zoomMode() const { return m_zoomMode; }

const QUrl &ImageZoomState::containerUrl() const { return m_containerUrl; }

ImageZoomSnapshot ImageZoomState::snapshot() const
{
    return ImageZoomSnapshot { m_imageSize, m_viewportSize, m_displaySize, m_zoomPercent,
        m_zoomMode, m_containerUrl };
}

bool ImageZoomState::setViewportSize(const QSizeF &viewportSize, qreal devicePixelRatio)
{
    const QSizeF normalizedViewportSize(
        std::max<qreal>(0.0, viewportSize.width()), std::max<qreal>(0.0, viewportSize.height()));
    if (imageZoomApproximatelyEqual(m_viewportSize, normalizedViewportSize)) {
        return false;
    }

    m_viewportSize = normalizedViewportSize;
    update(devicePixelRatio);
    return true;
}

bool ImageZoomState::setImageSize(const QSize &imageSize, qreal devicePixelRatio)
{
    if (m_imageSize == imageSize) {
        return false;
    }

    m_imageSize = imageSize;
    update(devicePixelRatio);
    return true;
}

bool ImageZoomState::setManualZoomPercent(qreal zoomPercent, qreal devicePixelRatio)
{
    if (!std::isfinite(zoomPercent)) {
        return false;
    }

    const qreal clampedZoomPercent
        = std::clamp(zoomPercent, minimumManualZoomPercent, maximumManualZoomPercent);
    const bool zoomChanged = !imageZoomApproximatelyEqual(m_zoomPercent, clampedZoomPercent);
    const bool modeChanged = m_zoomMode != ImageZoomMode::Manual;

    if (!zoomChanged && !modeChanged) {
        return false;
    }

    m_zoomMode = ImageZoomMode::Manual;
    m_zoomPercent = clampedZoomPercent;
    m_displaySize = displaySizeForZoomPercent(m_zoomPercent, devicePixelRatio);
    return true;
}

bool ImageZoomState::setFitMode(ImageZoomMode zoomMode, qreal devicePixelRatio)
{
    if (zoomMode == ImageZoomMode::Manual || m_zoomMode == zoomMode) {
        return false;
    }

    m_zoomMode = zoomMode;
    update(devicePixelRatio);
    return true;
}

void ImageZoomState::resetZoom(qreal devicePixelRatio)
{
    m_zoomMode = ImageZoomMode::Fit;
    update(devicePixelRatio);
}

void ImageZoomState::prepareImageContainer(const QUrl &containerUrl)
{
    if (!sameContainerNavigationUrl(containerUrl, m_containerUrl)) {
        m_zoomMode = ImageZoomMode::Fit;
    }
    m_containerUrl = containerUrl;
}

LoadedImageZoom ImageZoomState::loadedImageZoom(
    const QUrl &containerUrl, const QSize &imageSize, qreal devicePixelRatio) const
{
    ImageZoomMode loadedZoomMode = ImageZoomMode::Fit;
    if (sameContainerNavigationUrl(containerUrl, m_containerUrl)) {
        loadedZoomMode = m_zoomMode;
    }

    const qreal loadedZoomPercent = loadedZoomMode == ImageZoomMode::Manual
        ? m_zoomPercent
        : fitZoomPercentForImageSize(loadedZoomMode, imageSize, devicePixelRatio);
    return LoadedImageZoom { loadedZoomMode, loadedZoomPercent,
        displaySizeForZoomPercent(loadedZoomPercent, imageSize, devicePixelRatio) };
}

void ImageZoomState::setLoadedSvgZoom(const QUrl &containerUrl, const LoadedImageZoom &zoom)
{
    m_zoomMode = zoom.zoomMode;
    m_zoomPercent = zoom.zoomPercent;
    m_displaySize = QSizeF(std::max<qreal>(0.0, zoom.displaySize.width()),
        std::max<qreal>(0.0, zoom.displaySize.height()));
    m_containerUrl = containerUrl;
}

void ImageZoomState::clearContainer() { m_containerUrl = QUrl(); }

void ImageZoomState::update(qreal devicePixelRatio)
{
    if (m_zoomMode != ImageZoomMode::Manual) {
        m_zoomPercent = fitZoomPercent(m_zoomMode, devicePixelRatio);
    }

    m_displaySize = displaySizeForZoomPercent(m_zoomPercent, devicePixelRatio);
}

qreal ImageZoomState::fitZoomPercent(ImageZoomMode zoomMode, qreal devicePixelRatio) const
{
    return fitZoomPercentForImageSize(zoomMode, m_imageSize, devicePixelRatio);
}

qreal ImageZoomState::fitZoomPercentForImageSize(
    ImageZoomMode zoomMode, const QSize &imageSize, qreal devicePixelRatio) const
{
    const qreal fallbackZoomPercent = devicePixelRatio * 100.0;

    if (imageSize.isEmpty()) {
        return fallbackZoomPercent;
    }

    const auto fitWidthZoomPercent = [this, devicePixelRatio, fallbackZoomPercent, imageSize]() {
        if (m_viewportSize.width() <= 0.0) {
            return fallbackZoomPercent;
        }

        return std::max<qreal>(
            0.0, m_viewportSize.width() * devicePixelRatio * 100.0 / imageSize.width());
    };
    const auto fitHeightZoomPercent = [this, devicePixelRatio, fallbackZoomPercent, imageSize]() {
        if (m_viewportSize.height() <= 0.0) {
            return fallbackZoomPercent;
        }

        return std::max<qreal>(
            0.0, m_viewportSize.height() * devicePixelRatio * 100.0 / imageSize.height());
    };

    switch (zoomMode) {
    case ImageZoomMode::Fit:
        if (m_viewportSize.isEmpty()) {
            return fallbackZoomPercent;
        }
        return std::min(fitWidthZoomPercent(), fitHeightZoomPercent());
    case ImageZoomMode::FitHeight:
        return fitHeightZoomPercent();
    case ImageZoomMode::FitWidth:
        return fitWidthZoomPercent();
    case ImageZoomMode::Manual:
        return m_zoomPercent;
    }

    return fallbackZoomPercent;
}

QSizeF ImageZoomState::displaySizeForZoomPercent(qreal zoomPercent, qreal devicePixelRatio) const
{
    return displaySizeForZoomPercent(zoomPercent, m_imageSize, devicePixelRatio);
}

QSizeF ImageZoomState::displaySizeForZoomPercent(
    qreal zoomPercent, const QSize &imageSize, qreal devicePixelRatio) const
{
    if (imageSize.isEmpty() || !std::isfinite(zoomPercent) || zoomPercent <= 0.0) {
        return {};
    }

    const qreal scale = zoomPercent / (devicePixelRatio * 100.0);
    return QSizeF(imageSize.width() * scale, imageSize.height() * scale);
}
}
