// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagespreadzoomcontroller.h"

#include "imagepresentationcontroller.h"
#include "imagerendering.h"
#include "imagespreadgeometry.h"

#include <utility>

namespace KiriView {
ImageSpreadZoomController::ImageSpreadZoomController(RenderContextProvider renderContextProvider,
    ImagePresentationController &primaryPresentation,
    ImagePresentationController &secondaryPresentation)
    : m_renderContextProvider(std::move(renderContextProvider))
    , m_primaryPresentation(primaryPresentation)
    , m_secondaryPresentation(secondaryPresentation)
{
}

QRectF ImageSpreadZoomController::visibleItemRect() const { return m_visibleItemRect; }

void ImageSpreadZoomController::setVisibleItemRect(const QRectF &visibleItemRect)
{
    m_visibleItemRect = visibleItemRect;
}

QSizeF ImageSpreadZoomController::displaySize() const { return m_zoomState.displaySize(); }

QSizeF ImageSpreadZoomController::primaryDisplaySize() const
{
    return imageSpreadScaledPageDisplaySize(
        m_primaryPresentation.imageSize(), spreadImageSize(), displaySize());
}

QSizeF ImageSpreadZoomController::secondaryDisplaySize() const
{
    return imageSpreadScaledPageDisplaySize(
        m_secondaryPresentation.imageSize(), spreadImageSize(), displaySize());
}

qreal ImageSpreadZoomController::zoomPercent() const { return m_zoomState.zoomPercent(); }

ImageZoomMode ImageSpreadZoomController::zoomMode() const { return m_zoomState.zoomMode(); }

qreal ImageSpreadZoomController::maximumManualZoomPercent() const
{
    return m_zoomState.maximumManualZoomPercent(devicePixelRatio());
}

qreal ImageSpreadZoomController::clampedManualZoomPercent(qreal zoomPercent) const
{
    return m_zoomState.clampedManualZoomPercent(zoomPercent, devicePixelRatio());
}

qreal ImageSpreadZoomController::steppedManualZoomPercent(qreal stepCount) const
{
    return m_zoomState.steppedManualZoomPercent(stepCount, devicePixelRatio());
}

QSize ImageSpreadZoomController::spreadImageSize() const
{
    return imageSpreadImageSize(
        m_primaryPresentation.imageSize(), m_secondaryPresentation.imageSize());
}

void ImageSpreadZoomController::clearZoomState() { m_zoomState = ImageZoomState {}; }

void ImageSpreadZoomController::setZoomPercent(qreal zoomPercent, bool rightToLeftReading)
{
    m_zoomState.setManualZoomPercent(zoomPercent, devicePixelRatio());
    applyZoomPercentToPages();
    applyVisibleItemRects(rightToLeftReading);
}

bool ImageSpreadZoomController::setFitMode(ImageZoomMode zoomMode, bool rightToLeftReading)
{
    if (zoomMode == ImageZoomMode::Manual) {
        return false;
    }

    m_zoomState.setFitMode(zoomMode, devicePixelRatio());
    applyZoomPercentToPages();
    applyVisibleItemRects(rightToLeftReading);
    return true;
}

void ImageSpreadZoomController::resetZoom(bool rightToLeftReading)
{
    m_zoomState.resetZoom(devicePixelRatio());
    applyZoomPercentToPages();
    applyVisibleItemRects(rightToLeftReading);
}

void ImageSpreadZoomController::updateFromPrimaryPresentation(bool rightToLeftReading)
{
    const QSize nextSpreadImageSize = spreadImageSize();
    const qreal currentDevicePixelRatio = devicePixelRatio();
    const ImageZoomMode activeZoomMode = m_zoomState.imageSize().isEmpty()
        ? m_primaryPresentation.zoomMode()
        : m_zoomState.zoomMode();
    const qreal activeZoomPercent = m_zoomState.imageSize().isEmpty()
        ? m_primaryPresentation.zoomPercent()
        : m_zoomState.zoomPercent();

    m_zoomState.setViewportSize(m_primaryPresentation.viewportSize(), currentDevicePixelRatio);
    m_zoomState.setImageSize(nextSpreadImageSize, currentDevicePixelRatio);
    if (activeZoomMode == ImageZoomMode::Manual) {
        m_zoomState.setManualZoomPercent(activeZoomPercent, currentDevicePixelRatio);
    } else {
        m_zoomState.setFitMode(activeZoomMode, currentDevicePixelRatio);
    }

    applyZoomPercentToPages();
    applyVisibleItemRects(rightToLeftReading);
}

void ImageSpreadZoomController::updateRenderContext(bool rightToLeftReading)
{
    m_zoomState.update(devicePixelRatio());
    applyVisibleItemRects(rightToLeftReading);
}

void ImageSpreadZoomController::applyVisibleItemRects(bool rightToLeftReading)
{
    const QRectF primaryRect = primaryPageRect(rightToLeftReading);
    const QRectF secondaryRect = secondaryPageRect(rightToLeftReading);
    m_primaryPresentation.setVisibleItemRect(
        imageSpreadVisiblePageRect(m_visibleItemRect, primaryRect));
    m_secondaryPresentation.setVisibleItemRect(
        imageSpreadVisiblePageRect(m_visibleItemRect, secondaryRect));
}

void ImageSpreadZoomController::applyZoomToPrimaryPage(ImageZoomMode zoomMode, qreal zoomPercent)
{
    if (zoomMode == ImageZoomMode::Manual) {
        m_primaryPresentation.setZoomPercent(zoomPercent);
    } else if (zoomMode == ImageZoomMode::Fit) {
        m_primaryPresentation.resetZoom();
    } else {
        m_primaryPresentation.setFitMode(zoomMode);
    }
}

void ImageSpreadZoomController::applyStoredZoomToPrimaryPage()
{
    if (m_zoomState.imageSize().isEmpty()) {
        return;
    }

    applyZoomToPrimaryPage(m_zoomState.zoomMode(), m_zoomState.zoomPercent());
}

QRectF ImageSpreadZoomController::primaryPageRect(bool rightToLeftReading) const
{
    return imageSpreadPrimaryPageRect(
        primaryDisplaySize(), secondaryDisplaySize(), displaySize(), rightToLeftReading);
}

QRectF ImageSpreadZoomController::secondaryPageRect(bool rightToLeftReading) const
{
    return imageSpreadSecondaryPageRect(
        primaryDisplaySize(), secondaryDisplaySize(), displaySize(), rightToLeftReading);
}

void ImageSpreadZoomController::applyZoomPercentToPages()
{
    const qreal nextZoomPercent = m_zoomState.zoomPercent();
    m_primaryPresentation.setZoomPercent(nextZoomPercent);
    m_secondaryPresentation.setZoomPercent(nextZoomPercent);
}

ImageDocumentRenderContext ImageSpreadZoomController::renderContext() const
{
    ImageDocumentRenderContext context
        = m_renderContextProvider ? m_renderContextProvider() : ImageDocumentRenderContext {};
    return normalizedImageDocumentRenderContext(context);
}

qreal ImageSpreadZoomController::devicePixelRatio() const
{
    return renderContext().devicePixelRatio;
}
}
