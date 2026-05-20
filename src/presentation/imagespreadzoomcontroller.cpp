// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadzoomcontroller.h"

#include "presentation/imagepresentationcontroller.h"
#include "presentation/imagespreadgeometry.h"

#include <utility>

namespace {
bool hasZoomStateChange(const KiriView::ImageZoomChangeSet &changes)
{
    return changes.imageSizeChanged || changes.viewportSizeChanged || changes.zoomModeChanged
        || changes.zoomPercentChanged || changes.displaySizeChanged
        || changes.maximumManualZoomPercentChanged;
}
}

namespace KiriView {
ImageSpreadZoomController::ImageSpreadZoomController(RenderContextProvider renderContextProvider,
    ImagePresentationController &primaryPresentation,
    ImagePresentationController &secondaryPresentation)
    : m_renderContextState(std::move(renderContextProvider))
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

ImageZoomChangeSet ImageSpreadZoomController::setZoomPercent(
    qreal zoomPercent, bool rightToLeftReading)
{
    return mutateZoomState(
        [zoomPercent](ImageZoomState &zoomState, qreal devicePixelRatio) {
            zoomState.setManualZoomPercent(zoomPercent, devicePixelRatio);
        },
        rightToLeftReading);
}

ImageZoomChangeSet ImageSpreadZoomController::setFitMode(
    ImageZoomMode zoomMode, bool rightToLeftReading)
{
    if (zoomMode == ImageZoomMode::Manual) {
        return {};
    }

    return mutateZoomState(
        [zoomMode](ImageZoomState &zoomState, qreal devicePixelRatio) {
            zoomState.setFitMode(zoomMode, devicePixelRatio);
        },
        rightToLeftReading);
}

ImageZoomChangeSet ImageSpreadZoomController::resetZoom(bool rightToLeftReading)
{
    return mutateZoomState([](ImageZoomState &zoomState,
                               qreal devicePixelRatio) { zoomState.resetZoom(devicePixelRatio); },
        rightToLeftReading);
}

ImageZoomChangeSet ImageSpreadZoomController::updateFromPrimaryPresentation(bool rightToLeftReading)
{
    const QSize nextSpreadImageSize = spreadImageSize();
    const ImageZoomMode activeZoomMode = m_zoomState.imageSize().isEmpty()
        ? m_primaryPresentation.zoomMode()
        : m_zoomState.zoomMode();
    const qreal activeZoomPercent = m_zoomState.imageSize().isEmpty()
        ? m_primaryPresentation.zoomPercent()
        : m_zoomState.zoomPercent();

    return mutateZoomState(
        [this, nextSpreadImageSize, activeZoomMode, activeZoomPercent](
            ImageZoomState &zoomState, qreal devicePixelRatio) {
            zoomState.setViewportSize(m_primaryPresentation.viewportSize(), devicePixelRatio);
            zoomState.setImageSize(nextSpreadImageSize, devicePixelRatio);
            if (activeZoomMode == ImageZoomMode::Manual) {
                zoomState.setManualZoomPercent(activeZoomPercent, devicePixelRatio);
                return;
            }

            zoomState.setFitMode(activeZoomMode, devicePixelRatio);
        },
        rightToLeftReading);
}

ImageZoomChangeSet ImageSpreadZoomController::updateRenderContext(bool rightToLeftReading)
{
    return mutateZoomState([](ImageZoomState &zoomState,
                               qreal devicePixelRatio) { zoomState.update(devicePixelRatio); },
        rightToLeftReading);
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

ImageZoomChangeSet ImageSpreadZoomController::mutateZoomState(
    const ZoomStateMutation &mutation, bool rightToLeftReading)
{
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    const ImageRenderContextChange renderContextChange = m_renderContextState.refresh();

    mutation(m_zoomState, renderContextChange.current.devicePixelRatio);

    const ImageZoomChangeSet changes
        = ImageZoomState::changeSet(previous, renderContextChange.previous.devicePixelRatio,
            m_zoomState.snapshot(), renderContextChange.current.devicePixelRatio, false);
    if (!hasZoomStateChange(changes)) {
        return changes;
    }

    applyZoomPercentToPages();
    applyVisibleItemRects(rightToLeftReading);
    return changes;
}

qreal ImageSpreadZoomController::devicePixelRatio() const
{
    return m_renderContextState.devicePixelRatio();
}
}
