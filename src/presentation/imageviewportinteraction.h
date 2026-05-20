// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEVIEWPORTINTERACTION_H
#define KIRIVIEW_IMAGEVIEWPORTINTERACTION_H

#include "presentation/imageviewportscanstate.h"

#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <QtGlobal>

namespace KiriView {
struct ImageViewportInteractionSnapshot {
    QSize imageSize;
    QSizeF viewportSize;
    QSizeF displaySize;
    qreal devicePixelRatio = 1.0;
    bool rightToLeftReadingActive = false;
};

class ImageViewportInteraction final
{
public:
    QPointF panContentPosition(const ImageViewportInteractionSnapshot &snapshot,
        const QPointF &contentPosition, const QPointF &delta) const;
    QPointF nextScanContentPosition(
        const ImageViewportInteractionSnapshot &snapshot, const QPointF &contentPosition) const;
    QPointF previousScanContentPosition(
        const ImageViewportInteractionSnapshot &snapshot, const QPointF &contentPosition) const;
    QPointF initialScanContentPosition(const ImageViewportInteractionSnapshot &snapshot) const;
    QPointF finalScanContentPosition(const ImageViewportInteractionSnapshot &snapshot) const;
    void requestNextDisplayedImageFinalScanStart();
    ImageViewportScanStart beginDisplayedImage();
    void cancelPendingDisplayedImageStart();
    QPointF displayedImageInitialContentPosition(
        const ImageViewportInteractionSnapshot &snapshot) const;
    bool viewportPointInsideImage(const ImageViewportInteractionSnapshot &snapshot,
        const QPointF &contentPosition, const QPointF &viewportPoint) const;
    QPointF zoomContentPosition(const ImageViewportInteractionSnapshot &snapshot,
        const QPointF &contentPosition, const QPointF &viewportAnchorPoint,
        qreal nextZoomPercent) const;
    ImageViewportScanStart displayedImageScanStart() const;
    bool pendingFinalScanStart() const;

private:
    ImageViewportScanState m_scanState;
};
}

#endif
