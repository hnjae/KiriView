// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiriimageviewportcommandbridge.h"

#include <QVariant>

KiriImageViewportCommandBridge::KiriImageViewportCommandBridge(QQuickItem* parent)
    : QQuickItem(parent)
{
}

KiriImageViewportCommandBridge::~KiriImageViewportCommandBridge()
{
    disconnectDocument();
    disconnectTarget();
}

KiriImageDocument* KiriImageViewportCommandBridge::document() const { return m_document; }

void KiriImageViewportCommandBridge::setDocument(KiriImageDocument* document)
{
    if (m_document == document) {
        return;
    }

    disconnectDocument();
    m_document = document;
    connectDocument();
    Q_EMIT documentChanged();
    applyViewportProjection();
}

QQuickItem* KiriImageViewportCommandBridge::target() const { return m_target; }

void KiriImageViewportCommandBridge::setTarget(QQuickItem* target)
{
    if (m_target == target) {
        return;
    }

    disconnectTarget();
    m_target = target;
    if (m_target != nullptr) {
        m_targetDestroyedConnection = connect(m_target, &QObject::destroyed, this, [this]() {
            m_target = nullptr;
            m_targetDestroyedConnection = {};
            Q_EMIT targetChanged();
        });
    }
    Q_EMIT targetChanged();
    applyViewportProjection();
}

bool KiriImageViewportCommandBridge::active() const { return m_active; }

void KiriImageViewportCommandBridge::setActive(bool active)
{
    if (m_active == active) {
        return;
    }

    m_active = active;
    Q_EMIT activeChanged();
    applyViewportProjection();
}

bool KiriImageViewportCommandBridge::applying() const { return m_applying; }

QPointF KiriImageViewportCommandBridge::currentContentPosition() const
{
    if (m_target == nullptr) {
        return {};
    }

    return QPointF(
        m_target->property("contentX").toReal(), m_target->property("contentY").toReal());
}

bool KiriImageViewportCommandBridge::requestContentPosition(QPointF contentPosition)
{
    if (!bridgeReady()) {
        return false;
    }

    const QPointF previousContentPosition = currentContentPosition();
    m_document->requestViewportContentPosition(contentPosition);
    const bool commandAdvanced
        = m_document->viewportCommandRevisionNewerThan(m_appliedViewportCommandRevisionToken);
    return previousContentPosition != currentContentPosition() || commandAdvanced;
}

bool KiriImageViewportCommandBridge::observeViewportContentPosition(
    KiriImageDocument::ViewportObservationOrigin origin)
{
    if (!bridgeReady() || m_applying) {
        return false;
    }

    return m_document->observeViewportContentPosition(currentContentPosition(), origin);
}

void KiriImageViewportCommandBridge::applyViewportProjection()
{
    if (!bridgeReady() || m_applying) {
        return;
    }

    const QString commandRevisionToken = m_document->viewportCommandRevisionToken();
    if (m_document->viewportCommandStatus()
        == static_cast<int>(KiriImageDocument::ViewportCommandStatus::Rejected)) {
        return;
    }
    if (!m_document->viewportProjectionNewerThan(
            m_appliedViewportCommandRevisionToken, m_appliedViewportObservationRevisionToken)) {
        return;
    }

    setApplying(true);
    const bool commandNewer
        = m_document->viewportCommandRevisionNewerThan(m_appliedViewportCommandRevisionToken);
    if (commandNewer) {
        m_document->beginViewportCommandApplication(commandRevisionToken);
    }
    setContentPosition(m_document->viewportContentPosition());
    if (commandNewer) {
        const QPointF actualContentPosition = currentContentPosition();
        m_document->completeViewportCommandApplication(commandRevisionToken, actualContentPosition);
        m_appliedViewportCommandRevisionToken = commandRevisionToken;
        m_document->acknowledgeViewportCommand(commandRevisionToken, actualContentPosition);
    }
    m_appliedViewportObservationRevisionToken = m_document->viewportObservationRevisionToken();
    setApplying(false);
}

bool KiriImageViewportCommandBridge::bridgeReady() const
{
    return m_active && m_document != nullptr && m_target != nullptr;
}

bool KiriImageViewportCommandBridge::contentPositionChanged(QPointF contentPosition) const
{
    return currentContentPosition() != contentPosition;
}

bool KiriImageViewportCommandBridge::setContentPosition(QPointF contentPosition)
{
    if (m_target == nullptr) {
        return false;
    }

    const bool moved = contentPositionChanged(contentPosition);
    m_target->setProperty("contentX", contentPosition.x());
    m_target->setProperty("contentY", contentPosition.y());
    return moved;
}

void KiriImageViewportCommandBridge::connectDocument()
{
    if (m_document == nullptr) {
        return;
    }

    m_documentDestroyedConnection = connect(m_document, &QObject::destroyed, this, [this]() {
        m_document = nullptr;
        m_documentDestroyedConnection = {};
        m_viewportFrameChangedConnection = {};
        Q_EMIT documentChanged();
    });
    m_viewportFrameChangedConnection = connect(m_document, &KiriImageDocument::viewportFrameChanged,
        this, &KiriImageViewportCommandBridge::applyViewportProjection);
}

void KiriImageViewportCommandBridge::disconnectDocument()
{
    m_document = nullptr;
    QObject::disconnect(m_documentDestroyedConnection);
    QObject::disconnect(m_viewportFrameChangedConnection);
    m_documentDestroyedConnection = {};
    m_viewportFrameChangedConnection = {};
}

void KiriImageViewportCommandBridge::disconnectTarget()
{
    m_target = nullptr;
    QObject::disconnect(m_targetDestroyedConnection);
    m_targetDestroyedConnection = {};
}

void KiriImageViewportCommandBridge::setApplying(bool applying)
{
    if (m_applying == applying) {
        return;
    }

    m_applying = applying;
    Q_EMIT applyingChanged();
}
