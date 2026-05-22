// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEOSOURCELOADRUNTIME_H
#define KIRIVIEW_VIDEOSOURCELOADRUNTIME_H

#include "async/imageasyncoperationstate.h"
#include "video/videoplaybackurlresolver.h"

#include <QUrl>
#include <functional>
#include <memory>

class QObject;

namespace KiriView {
struct VideoSourceLoadOperations {
    std::function<void()> clearPlaybackSource;
    std::function<void()> sourceCleared;
    std::function<void(const QUrl &)> sourceLoadStarted;
    std::function<void(const VideoPlaybackUrlResolution &)> playbackUrlReady;
    std::function<void(const QUrl &, const QString &)> sourceLoadFailed;
};

class VideoSourceLoadRuntime final
{
public:
    explicit VideoSourceLoadRuntime(std::unique_ptr<VideoPlaybackUrlResolver> resolver = {});
    ~VideoSourceLoadRuntime();

    bool active() const;
    void setSourceUrl(
        const QUrl &sourceUrl, QObject *receiver, VideoSourceLoadOperations operations);
    void shutdown();

private:
    void cancelAndCleanup();
    void completePlaybackUrlResolution(
        const VideoPlaybackUrlResolution &resolution, const VideoSourceLoadOperations &operations);
    void failPlaybackUrlResolution(quint64 operationId, const QUrl &sourceUrl,
        const QString &errorString, const VideoSourceLoadOperations &operations);

    std::unique_ptr<VideoPlaybackUrlResolver> m_resolver;
    ImageAsyncScopedOperationState<QUrl> m_resolution;
    bool m_shutdown = false;
};
}

#endif
