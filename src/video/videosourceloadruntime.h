// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEOSOURCELOADRUNTIME_H
#define KIRIVIEW_VIDEOSOURCELOADRUNTIME_H

#include "async/imageasyncoperationstate.h"
#include "video/videoplaybackurlresolver.h"
#include "video/videosourceloadplan.h"

#include <QString>
#include <QUrl>
#include <functional>
#include <memory>

class QObject;

namespace KiriView {
using VideoSourceLoadPlanCallback = std::function<void(VideoSourceLoadPlan)>;

class VideoSourceLoadRuntime final
{
public:
    explicit VideoSourceLoadRuntime(std::unique_ptr<VideoPlaybackUrlResolver> resolver = {});
    ~VideoSourceLoadRuntime();

    bool active() const;
    void setSourceUrl(
        const QUrl &sourceUrl, QObject *receiver, VideoSourceLoadPlanCallback planCallback);
    void shutdown();

private:
    void cancelAndCleanup();
    void completePlaybackUrlResolution(
        const VideoPlaybackUrlResolution &resolution, const VideoSourceLoadPlanCallback &callback);
    void failPlaybackUrlResolution(quint64 operationId, const QUrl &sourceUrl,
        const QString &errorString, const VideoSourceLoadPlanCallback &callback);

    std::unique_ptr<VideoPlaybackUrlResolver> m_resolver;
    ImageAsyncScopedOperationState<QUrl> m_resolution;
    bool m_shutdown = false;
};
}

#endif
