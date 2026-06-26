// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videooutputrendercontextobserver.h"

#include <QEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QWindow>
#include <cmath>
#include <utility>

namespace kiriview {
VideoOutputRenderContextObserver::VideoOutputRenderContextObserver(
    QObject* parent, std::function<void()> renderContextChanged)
    : QObject(parent)
    , m_renderContextChanged(std::move(renderContextChanged))
{
}

void VideoOutputRenderContextObserver::setVideoOutput(QObject* videoOutput)
{
    auto* item = qobject_cast<QQuickItem*>(videoOutput);
    if (m_videoOutputItem.data() == item) {
        setObservedWindow(item == nullptr ? nullptr : item->window());
        notifyRenderContextChanged();
        return;
    }

    clearVideoOutputConnections();
    setObservedWindow(nullptr);
    m_videoOutputItem = item;

    if (item != nullptr) {
        m_videoOutputConnections.push_back(
            QObject::connect(item, &QObject::destroyed, this, [this]() {
                m_videoOutputItem.clear();
                setObservedWindow(nullptr);
                notifyRenderContextChanged();
            }));
        m_videoOutputConnections.push_back(
            QObject::connect(item, &QQuickItem::windowChanged, this, [this](QQuickWindow* window) {
                setObservedWindow(window);
                notifyRenderContextChanged();
            }));
        m_videoOutputConnections.push_back(QObject::connect(
            item, &QQuickItem::widthChanged, this, [this]() { notifyRenderContextChanged(); }));
        m_videoOutputConnections.push_back(QObject::connect(
            item, &QQuickItem::heightChanged, this, [this]() { notifyRenderContextChanged(); }));
        setObservedWindow(item->window());
    }

    notifyRenderContextChanged();
}

std::optional<qreal> VideoOutputRenderContextObserver::devicePixelRatio() const
{
    if (m_window == nullptr) {
        return std::nullopt;
    }

    const qreal ratio = m_window->effectiveDevicePixelRatio();
    if (!std::isfinite(ratio) || ratio <= 0.0) {
        return std::nullopt;
    }

    return ratio;
}

bool VideoOutputRenderContextObserver::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_window.data() && event->type() == QEvent::DevicePixelRatioChange) {
        notifyRenderContextChanged();
    }

    return QObject::eventFilter(watched, event);
}

void VideoOutputRenderContextObserver::clearVideoOutputConnections()
{
    for (const QMetaObject::Connection& connection : m_videoOutputConnections) {
        QObject::disconnect(connection);
    }
    m_videoOutputConnections.clear();
    m_videoOutputItem.clear();
}

void VideoOutputRenderContextObserver::setObservedWindow(QQuickWindow* window)
{
    if (m_window.data() == window) {
        return;
    }

    clearWindowConnections();
    m_window = window;
    if (m_window == nullptr) {
        return;
    }

    m_window->installEventFilter(this);
    m_windowConnections.push_back(
        QObject::connect(m_window.data(), &QObject::destroyed, this, [this]() {
            m_window.clear();
            m_windowConnections.clear();
            notifyRenderContextChanged();
        }));
    m_windowConnections.push_back(QObject::connect(m_window.data(), &QWindow::screenChanged, this,
        [this](QScreen*) { notifyRenderContextChanged(); }));
}

void VideoOutputRenderContextObserver::clearWindowConnections()
{
    for (const QMetaObject::Connection& connection : m_windowConnections) {
        QObject::disconnect(connection);
    }
    m_windowConnections.clear();

    if (m_window != nullptr) {
        m_window->removeEventFilter(this);
        m_window.clear();
    }
}

void VideoOutputRenderContextObserver::notifyRenderContextChanged() const
{
    if (m_renderContextChanged) {
        m_renderContextChanged();
    }
}
}
