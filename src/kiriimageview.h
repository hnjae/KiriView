// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEVIEW_H
#define KIRIVIEW_KIRIIMAGEVIEW_H

#include "imagedocumenttypes.h"
#include "kiriimagedocument.h"

#include <QMetaObject>
#include <QPointF>
#include <QQuickItem>
#include <QSize>
#include <QSizeF>
#include <QtQml/qqmlregistration.h>

class KiriImageView : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(KiriImageDocument *document READ document WRITE setDocument NOTIFY documentChanged)

public:
    explicit KiriImageView(QQuickItem *parent = nullptr);
    ~KiriImageView() override;

    KiriImageDocument *document() const;
    void setDocument(KiriImageDocument *document);

    Q_INVOKABLE QPointF panContentPosition(
        const QPointF &contentPosition, const QPointF &delta) const;
    Q_INVOKABLE bool viewportPointInsideImage(
        const QPointF &contentPosition, const QPointF &viewportPoint) const;
    Q_INVOKABLE QPointF zoomContentPosition(const QPointF &contentPosition,
        const QPointF &viewportAnchorPoint, double nextZoomPercent) const;

    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;

Q_SIGNALS:
    void documentChanged();

private:
    QSize imageSize() const;
    QSizeF viewportSize() const;
    QSizeF displaySize() const;
    void connectDocument();
    void disconnectDocument();
    KiriView::ImageDocumentRenderContext renderContext() const;
    qreal displayDevicePixelRatio() const;
    int maximumTextureSize() const;

    KiriImageDocument *m_document = nullptr;
    QMetaObject::Connection m_repaintConnection;
    QMetaObject::Connection m_documentDestroyedConnection;
};

#endif
