// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videooutputruntime.h"

#include "video/videooutputrendercontextobserver.h"
#include "video/videozoomstate.h"

#include <QObject>
#include <utility>

namespace kiriview {
VideoOutputRuntime::VideoOutputRuntime(QObject *context, VideoOutputRuntimeCallbacks callbacks)
    : m_callbacks(std::move(callbacks))
    , m_renderContextObserver(std::make_unique<VideoOutputRenderContextObserver>(
          context, [this]() { notifyZoomProjectionChanged(); }))
{
}

VideoOutputRuntime::~VideoOutputRuntime()
{
    disconnectVideoOutputDestroyed();
    setBackendVideoOutput(nullptr);
}

QObject *VideoOutputRuntime::videoOutput() const { return m_videoOutput.data(); }

void VideoOutputRuntime::setVideoOutput(QObject *videoOutput)
{
    if (m_videoOutput.data() == videoOutput) {
        setBackendVideoOutput(videoOutput);
        return;
    }

    disconnectVideoOutputDestroyed();
    m_videoOutput = videoOutput;
    setBackendVideoOutput(videoOutput);
    m_renderContextObserver->setVideoOutput(videoOutput);
    connectVideoOutputDestroyed(videoOutput);
    notifyVideoOutputChanged();
}

void VideoOutputRuntime::setVideoOutputGeometry(const QRectF &contentRect, const QRectF &sourceRect)
{
    if (m_contentRect == contentRect && m_sourceRect == sourceRect) {
        return;
    }

    m_contentRect = contentRect;
    m_sourceRect = sourceRect;
    notifyZoomProjectionChanged();
}

std::optional<int> VideoOutputRuntime::zoomPercent() const
{
    const std::optional<qreal> devicePixelRatio = m_renderContextObserver->devicePixelRatio();
    if (!devicePixelRatio.has_value()) {
        return std::nullopt;
    }

    return videoZoomPercentForRects(m_contentRect, m_sourceRect, devicePixelRatio.value());
}

void VideoOutputRuntime::connectVideoOutputDestroyed(QObject *videoOutput)
{
    if (videoOutput == nullptr) {
        return;
    }

    m_videoOutputDestroyedConnection = QObject::connect(
        videoOutput, &QObject::destroyed, m_renderContextObserver.get(), [this]() {
            m_videoOutput.clear();
            setBackendVideoOutput(nullptr);
            m_renderContextObserver->setVideoOutput(nullptr);
            notifyVideoOutputChanged();
        });
}

void VideoOutputRuntime::disconnectVideoOutputDestroyed()
{
    if (m_videoOutputDestroyedConnection) {
        QObject::disconnect(m_videoOutputDestroyedConnection);
        m_videoOutputDestroyedConnection = {};
    }
}

void VideoOutputRuntime::notifyVideoOutputChanged() const
{
    if (m_callbacks.videoOutputChanged) {
        m_callbacks.videoOutputChanged();
    }
}

void VideoOutputRuntime::notifyZoomProjectionChanged() const
{
    if (m_callbacks.zoomProjectionChanged) {
        m_callbacks.zoomProjectionChanged();
    }
}

void VideoOutputRuntime::setBackendVideoOutput(QObject *videoOutput) const
{
    if (m_callbacks.setBackendVideoOutput) {
        m_callbacks.setBackendVideoOutput(videoOutput);
    }
}
}
