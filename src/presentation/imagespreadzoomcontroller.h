// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADZOOMCONTROLLER_H
#define KIRIVIEW_IMAGESPREADZOOMCONTROLLER_H

#include "presentation/imagezoomworkflowstate.h"

#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QtGlobal>
#include <functional>

namespace KiriView {
class ImagePresentationController;

class ImageSpreadZoomController final
{
public:
    using RenderContextProvider = ImageZoomWorkflowState::RenderContextProvider;

    ImageSpreadZoomController(RenderContextProvider renderContextProvider,
        ImagePresentationController &primaryPresentation,
        ImagePresentationController &secondaryPresentation);

    QSizeF displaySize() const;
    QSizeF primaryDisplaySize() const;
    QSizeF secondaryDisplaySize() const;
    qreal zoomPercent() const;
    ImageZoomMode zoomMode() const;
    qreal maximumManualZoomPercent() const;
    qreal clampedManualZoomPercent(qreal zoomPercent) const;
    qreal steppedManualZoomPercent(qreal stepCount) const;
    QSize spreadImageSize() const;

    void clearZoomState();
    ImageZoomChangeSet setZoomPercent(qreal zoomPercent);
    ImageZoomChangeSet setFitMode(ImageZoomMode zoomMode);
    ImageZoomChangeSet resetZoom();
    ImageZoomChangeSet updateFromPrimaryPresentation();
    ImageZoomChangeSet updateRenderContext();
    void applyZoomStateToPages(const QRectF &visibleItemRect, bool rightToLeftReading);
    void applyVisibleItemRects(const QRectF &visibleItemRect, bool rightToLeftReading);
    void applyZoomToPrimaryPage(ImageZoomMode zoomMode, qreal zoomPercent);
    void applyStoredZoomToPrimaryPage();

private:
    using ZoomStateMutation = ImageZoomWorkflowState::ZoomStateMutation;

    QRectF primaryPageRect(bool rightToLeftReading) const;
    QRectF secondaryPageRect(bool rightToLeftReading) const;
    void applyZoomPercentToPages();
    ImageZoomChangeSet mutateZoomState(const ZoomStateMutation &mutation);
    qreal devicePixelRatio() const;

    ImageZoomWorkflowState m_zoomWorkflowState;
    ImagePresentationController &m_primaryPresentation;
    ImagePresentationController &m_secondaryPresentation;
};
}

#endif
