// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEOOUTPUTRENDERCONTEXTOBSERVER_H
#define KIRIVIEW_VIDEOOUTPUTRENDERCONTEXTOBSERVER_H

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QtGlobal>
#include <functional>
#include <optional>
#include <vector>

class QEvent;
class QQuickItem;
class QQuickWindow;

namespace kiriview {
class VideoOutputRenderContextObserver final : public QObject
{
public:
    explicit VideoOutputRenderContextObserver(
        QObject *parent = nullptr, std::function<void()> renderContextChanged = {});

    void setVideoOutput(QObject *videoOutput);
    std::optional<qreal> devicePixelRatio() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void clearVideoOutputConnections();
    void setObservedWindow(QQuickWindow *window);
    void clearWindowConnections();
    void notifyRenderContextChanged() const;

    std::function<void()> m_renderContextChanged;
    QPointer<QQuickItem> m_videoOutputItem;
    QPointer<QQuickWindow> m_window;
    std::vector<QMetaObject::Connection> m_videoOutputConnections;
    std::vector<QMetaObject::Connection> m_windowConnections;
};
}

#endif
