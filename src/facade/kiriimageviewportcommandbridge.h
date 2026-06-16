// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEVIEWPORTCOMMANDBRIDGE_H
#define KIRIVIEW_KIRIIMAGEVIEWPORTCOMMANDBRIDGE_H

#include "facade/kiriimagedocument.h"

#include <QMetaObject>
#include <QPointF>
#include <QQuickItem>
#include <QString>
#include <QtQml/qqmlregistration.h>

class KiriImageViewportCommandBridge : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(KiriImageDocument *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(QQuickItem *target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(bool applying READ applying NOTIFY applyingChanged)

public:
    explicit KiriImageViewportCommandBridge(QQuickItem *parent = nullptr);
    ~KiriImageViewportCommandBridge() override;

    KiriImageDocument *document() const;
    void setDocument(KiriImageDocument *document);
    QQuickItem *target() const;
    void setTarget(QQuickItem *target);
    bool active() const;
    void setActive(bool active);
    bool applying() const;

    Q_INVOKABLE QPointF currentContentPosition() const;
    Q_INVOKABLE bool requestContentPosition(const QPointF &contentPosition);
    Q_INVOKABLE bool observeViewportContentPosition(
        KiriImageDocument::ViewportObservationOrigin origin);
    Q_INVOKABLE void applyViewportProjection();

Q_SIGNALS:
    void documentChanged();
    void targetChanged();
    void activeChanged();
    void applyingChanged();

private:
    bool bridgeReady() const;
    bool contentPositionChanged(const QPointF &contentPosition) const;
    bool setContentPosition(const QPointF &contentPosition);
    void connectDocument();
    void disconnectDocument();
    void disconnectTarget();
    void setApplying(bool applying);

    KiriImageDocument *m_document = nullptr;
    QQuickItem *m_target = nullptr;
    bool m_active = false;
    bool m_applying = false;
    QString m_appliedViewportCommandRevisionToken = QStringLiteral("0");
    QString m_appliedViewportObservationRevisionToken = QStringLiteral("0");
    QMetaObject::Connection m_documentDestroyedConnection;
    QMetaObject::Connection m_viewportFrameChangedConnection;
    QMetaObject::Connection m_targetDestroyedConnection;
};

#endif
