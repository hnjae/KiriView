// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEOOUTPUTRUNTIME_H
#define KIRIVIEW_VIDEOOUTPUTRUNTIME_H

#include <QMetaObject>
#include <QPointer>
#include <QRectF>
#include <functional>
#include <memory>
#include <optional>

class QObject;

namespace kiriview {
class VideoOutputRenderContextObserver;

struct VideoOutputRuntimeCallbacks {
    std::function<void(QObject *)> setBackendVideoOutput;
    std::function<void()> videoOutputChanged;
    std::function<void()> zoomProjectionChanged;
};

class VideoOutputRuntime final
{
public:
    explicit VideoOutputRuntime(QObject *context, VideoOutputRuntimeCallbacks callbacks = {});
    ~VideoOutputRuntime();

    QObject *videoOutput() const;
    void setVideoOutput(QObject *videoOutput);
    void setVideoOutputGeometry(const QRectF &contentRect, const QRectF &sourceRect);
    std::optional<int> zoomPercent() const;

private:
    void connectVideoOutputDestroyed(QObject *videoOutput);
    void disconnectVideoOutputDestroyed();
    void notifyVideoOutputChanged() const;
    void notifyZoomProjectionChanged() const;
    void setBackendVideoOutput(QObject *videoOutput) const;

    VideoOutputRuntimeCallbacks m_callbacks;
    std::unique_ptr<VideoOutputRenderContextObserver> m_renderContextObserver;
    QPointer<QObject> m_videoOutput;
    QRectF m_contentRect;
    QRectF m_sourceRect;
    QMetaObject::Connection m_videoOutputDestroyedConnection;
};
}

#endif
