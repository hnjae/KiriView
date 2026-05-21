// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEOPLAYBACKURLRESOLVER_H
#define KIRIVIEW_VIDEOPLAYBACKURLRESOLVER_H

#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <memory>

class QObject;

namespace KiriView {
struct VideoPlaybackUrlResolution {
    quint64 operationId = 0;
    QUrl sourceUrl;
    QUrl playbackUrl;
};

using VideoPlaybackUrlResolvedCallback = std::function<void(VideoPlaybackUrlResolution)>;
using VideoPlaybackUrlFailedCallback = std::function<void(quint64, QUrl, QString)>;

class VideoPlaybackUrlResolver
{
public:
    virtual ~VideoPlaybackUrlResolver() = default;

    virtual void resolve(quint64 operationId, const QUrl &sourceUrl, QObject *receiver,
        VideoPlaybackUrlResolvedCallback resolvedCallback,
        VideoPlaybackUrlFailedCallback failedCallback)
        = 0;
    virtual void cancel() = 0;
    virtual void cleanup() = 0;
};

bool videoPlaybackBackendCanConsumeUrl(const QUrl &url);
std::unique_ptr<VideoPlaybackUrlResolver> createDefaultVideoPlaybackUrlResolver();
}

#endif
