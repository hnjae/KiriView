// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageviewportinteraction.h"

#include "presentation/imageviewportgeometry.h"

namespace {
QRectF viewportImageRect(const kiriview::ImageViewportInteractionSnapshot &snapshot)
{
    return kiriview::imageViewportImageRect(snapshot.viewportSize, snapshot.displaySize);
}
}

namespace kiriview {
QPointF ImageViewportInteraction::panContentPosition(
    const ImageViewportInteractionSnapshot &snapshot, const QPointF &contentPosition,
    const QPointF &delta) const
{
    return imageViewportPanPosition(
        snapshot.viewportSize, viewportImageRect(snapshot), contentPosition, delta);
}

QPointF ImageViewportInteraction::nextScanContentPosition(
    const ImageViewportInteractionSnapshot &snapshot, const QPointF &contentPosition) const
{
    return imageViewportNextZScanPosition(snapshot.viewportSize, viewportImageRect(snapshot),
        contentPosition, snapshot.rightToLeftReadingActive);
}

QPointF ImageViewportInteraction::previousScanContentPosition(
    const ImageViewportInteractionSnapshot &snapshot, const QPointF &contentPosition) const
{
    return imageViewportPreviousZScanPosition(snapshot.viewportSize, viewportImageRect(snapshot),
        contentPosition, snapshot.rightToLeftReadingActive);
}

QPointF ImageViewportInteraction::initialScanContentPosition(
    const ImageViewportInteractionSnapshot &snapshot) const
{
    return imageViewportInitialZScanPosition(
        snapshot.viewportSize, viewportImageRect(snapshot), snapshot.rightToLeftReadingActive);
}

QPointF ImageViewportInteraction::finalScanContentPosition(
    const ImageViewportInteractionSnapshot &snapshot) const
{
    return imageViewportFinalZScanPosition(
        snapshot.viewportSize, viewportImageRect(snapshot), snapshot.rightToLeftReadingActive);
}

void ImageViewportInteraction::requestNextDisplayedImageFinalScanStart()
{
    m_scanState.requestNextDisplayedImageFinalScanStart();
}

ImageViewportScanStart ImageViewportInteraction::beginDisplayedImage()
{
    return m_scanState.beginDisplayedImage();
}

void ImageViewportInteraction::cancelPendingDisplayedImageStart()
{
    m_scanState.cancelPendingDisplayedImageStart();
}

QPointF ImageViewportInteraction::displayedImageInitialContentPosition(
    const ImageViewportInteractionSnapshot &snapshot) const
{
    if (m_scanState.displayedImageScanStart() == ImageViewportScanStart::Final) {
        return finalScanContentPosition(snapshot);
    }

    return initialScanContentPosition(snapshot);
}

bool ImageViewportInteraction::viewportPointInsideImage(
    const ImageViewportInteractionSnapshot &snapshot, const QPointF &contentPosition,
    const QPointF &viewportPoint) const
{
    return imageViewportPointInsideImage(
        contentPosition, viewportPoint, viewportImageRect(snapshot));
}

QPointF ImageViewportInteraction::nearestImageViewportPoint(
    const ImageViewportInteractionSnapshot &snapshot, const QPointF &contentPosition,
    const QPointF &viewportPoint) const
{
    return imageViewportNearestImagePoint(
        contentPosition, viewportPoint, viewportImageRect(snapshot));
}

QPointF ImageViewportInteraction::zoomContentPosition(
    const ImageViewportInteractionSnapshot &snapshot, const QPointF &contentPosition,
    const QPointF &viewportAnchorPoint, qreal nextZoomPercent) const
{
    const QSizeF nextDisplaySize = imageViewportDisplaySizeForZoom(
        snapshot.imageSize, nextZoomPercent, snapshot.devicePixelRatio);
    return imageViewportContentPositionForZoom(snapshot.viewportSize, snapshot.displaySize,
        nextDisplaySize, contentPosition, viewportAnchorPoint);
}

ImageViewportScanStart ImageViewportInteraction::displayedImageScanStart() const
{
    return m_scanState.displayedImageScanStart();
}

bool ImageViewportInteraction::pendingFinalScanStart() const
{
    return m_scanState.pendingFinalScanStart();
}
}
