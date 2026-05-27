// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEVIEW_H
#define KIRIVIEW_KIRIIMAGEVIEW_H

#include "facade/kiriimagedocument.h"
#include "presentation/imageviewportinteraction.h"
#include "presentation/imageviewrendercontextbinding.h"
#include "rendering/imagerendercontext.h"
#include "rendering/imagesurface.h"

#include <QMetaObject>
#include <QPointF>
#include <QQuickItem>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QtQml/qqmlregistration.h>

class KiriImageView : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(KiriImageDocument *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(
        bool secondaryPage READ secondaryPage WRITE setSecondaryPage NOTIFY secondaryPageChanged)

public:
    explicit KiriImageView(QQuickItem *parent = nullptr);
    ~KiriImageView() override;

    KiriImageDocument *document() const;
    void setDocument(KiriImageDocument *document);
    bool secondaryPage() const;
    void setSecondaryPage(bool secondaryPage);

    Q_INVOKABLE QPointF panContentPosition(
        const QPointF &contentPosition, const QPointF &delta) const;
    Q_INVOKABLE QPointF nextScanContentPosition(const QPointF &contentPosition) const;
    Q_INVOKABLE QPointF previousScanContentPosition(const QPointF &contentPosition) const;
    Q_INVOKABLE QPointF initialScanContentPosition() const;
    Q_INVOKABLE QPointF finalScanContentPosition() const;
    Q_INVOKABLE void setNextDisplayedImageStartToFinalScanPosition();
    Q_INVOKABLE QPointF displayedImageInitialContentPosition() const;
    Q_INVOKABLE bool viewportPointInsideImage(
        const QPointF &contentPosition, const QPointF &viewportPoint) const;
    Q_INVOKABLE QPointF nearestImageViewportPoint(
        const QPointF &contentPosition, const QPointF &viewportPoint) const;
    Q_INVOKABLE QPointF zoomContentPosition(const QPointF &contentPosition,
        const QPointF &viewportAnchorPoint, double nextZoomPercent) const;

    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;
    void classBegin() override;
    void componentComplete() override;

Q_SIGNALS:
    void documentChanged();
    void secondaryPageChanged();
    void displayedImageInitialContentPositionRequested();

private:
    KiriView::DisplayedPageRole displayedPageRole() const;
    KiriView::ImageViewRenderContextBindingInput renderContextBindingInput(
        bool documentAttached) const;
    KiriView::ImageViewportInteractionSnapshot viewportInteractionSnapshot() const;
    KiriView::DisplayedImageRenderSnapshot renderSnapshot() const;
    QSizeF viewportSize() const;
    QSizeF displaySize() const;
    bool rightToLeftReadingActive() const;
    void connectDocument();
    void disconnectDocument();
    void synchronizeRenderContextBinding(KiriImageDocument *document, bool documentAttached);
    void applyRenderContextBinding(
        KiriView::ImageViewRenderContextBindingAction action, KiriImageDocument *document);
    void handleDisplayedUrlChanged();
    void handleLoadingChanged();
    KiriView::ImageDocumentRenderContext renderContext() const;
    qreal displayDevicePixelRatio() const;
    int maximumTextureSize() const;

    KiriImageDocument *m_document = nullptr;
    bool m_secondaryPage = false;
    bool m_componentComplete = true;
    QMetaObject::Connection m_repaintConnection;
    QMetaObject::Connection m_displayedUrlChangedConnection;
    QMetaObject::Connection m_loadingChangedConnection;
    QMetaObject::Connection m_documentDestroyedConnection;
    KiriView::ImageViewportInteraction m_viewportInteraction;
    KiriView::ImageViewRenderContextBinding m_renderContextBinding;
};

#endif
