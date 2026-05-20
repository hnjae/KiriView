// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADZOOMCONTROLLER_H
#define KIRIVIEW_IMAGESPREADZOOMCONTROLLER_H

#include "document/imagedocumenttypes.h"
#include "imagerendercontextstate.h"
#include "imagezoomstate.h"

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
    using RenderContextProvider = std::function<ImageDocumentRenderContext()>;

    ImageSpreadZoomController(RenderContextProvider renderContextProvider,
        ImagePresentationController &primaryPresentation,
        ImagePresentationController &secondaryPresentation);

    QRectF visibleItemRect() const;
    void setVisibleItemRect(const QRectF &visibleItemRect);
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
    ImageZoomChangeSet setZoomPercent(qreal zoomPercent, bool rightToLeftReading);
    ImageZoomChangeSet setFitMode(ImageZoomMode zoomMode, bool rightToLeftReading);
    ImageZoomChangeSet resetZoom(bool rightToLeftReading);
    ImageZoomChangeSet updateFromPrimaryPresentation(bool rightToLeftReading);
    ImageZoomChangeSet updateRenderContext(bool rightToLeftReading);
    void applyVisibleItemRects(bool rightToLeftReading);
    void applyZoomToPrimaryPage(ImageZoomMode zoomMode, qreal zoomPercent);
    void applyStoredZoomToPrimaryPage();

private:
    using ZoomStateMutation = std::function<void(ImageZoomState &, qreal devicePixelRatio)>;

    QRectF primaryPageRect(bool rightToLeftReading) const;
    QRectF secondaryPageRect(bool rightToLeftReading) const;
    void applyZoomPercentToPages();
    ImageZoomChangeSet mutateZoomState(const ZoomStateMutation &mutation, bool rightToLeftReading);
    qreal devicePixelRatio() const;

    ImageRenderContextState m_renderContextState;
    ImagePresentationController &m_primaryPresentation;
    ImagePresentationController &m_secondaryPresentation;
    ImageZoomState m_zoomState;
    QRectF m_visibleItemRect;
};
}

#endif
