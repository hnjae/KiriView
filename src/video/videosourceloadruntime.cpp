// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videosourceloadruntime.h"

#include "async/imagecallback.h"

#include <utility>

namespace KiriView {
VideoSourceLoadRuntime::VideoSourceLoadRuntime(std::unique_ptr<VideoPlaybackUrlResolver> resolver)
    : m_resolver(
          resolver == nullptr ? createDefaultVideoPlaybackUrlResolver() : std::move(resolver))
{
}

VideoSourceLoadRuntime::~VideoSourceLoadRuntime() { shutdown(); }

bool VideoSourceLoadRuntime::active() const { return m_resolution.active(); }

void VideoSourceLoadRuntime::setSourceUrl(
    const QUrl &sourceUrl, QObject *receiver, VideoSourceLoadOperations operations)
{
    m_shutdown = false;
    cancelAndCleanup();
    invokeIfSet(operations.clearPlaybackSource);

    if (sourceUrl.isEmpty()) {
        invokeIfSet(operations.sourceCleared);
        return;
    }

    const ImageAsyncScopedOperation<QUrl> operation = m_resolution.start(sourceUrl);
    invokeIfSet(operations.sourceLoadStarted, sourceUrl);

    m_resolver->resolve(
        operation.operationId, sourceUrl, receiver,
        [this, operations](VideoPlaybackUrlResolution resolution) {
            completePlaybackUrlResolution(resolution, operations);
        },
        [this, operations](quint64 operationId, QUrl failedSourceUrl, QString errorString) {
            failPlaybackUrlResolution(operationId, failedSourceUrl, errorString, operations);
        });
}

void VideoSourceLoadRuntime::shutdown()
{
    if (m_shutdown) {
        return;
    }

    cancelAndCleanup();
    m_shutdown = true;
}

void VideoSourceLoadRuntime::cancelAndCleanup()
{
    m_resolution.cancel();
    if (m_resolver != nullptr) {
        m_resolver->cancel();
        m_resolver->cleanup();
    }
}

void VideoSourceLoadRuntime::completePlaybackUrlResolution(
    const VideoPlaybackUrlResolution &resolution, const VideoSourceLoadOperations &operations)
{
    if (!m_resolution.finish(resolution.operationId, resolution.sourceUrl)) {
        return;
    }

    invokeIfSet(operations.playbackUrlReady, resolution);
}

void VideoSourceLoadRuntime::failPlaybackUrlResolution(quint64 operationId, const QUrl &sourceUrl,
    const QString &errorString, const VideoSourceLoadOperations &operations)
{
    if (!m_resolution.finish(operationId, sourceUrl)) {
        return;
    }

    invokeIfSet(operations.sourceLoadFailed, sourceUrl, errorString);
}
}
