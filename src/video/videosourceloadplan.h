// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEOSOURCELOADPLAN_H
#define KIRIVIEW_VIDEOSOURCELOADPLAN_H

#include <QString>
#include <QUrl>
#include <variant>
#include <vector>

namespace kiriview {
struct ClearVideoPlaybackSourceOperation {
};

struct ResetClearedVideoSourceOperation {
};

struct ResetVideoSourceLoadOperation {
    QUrl sourceUrl;
};

struct ApplyVideoPlaybackUrlOperation {
    QUrl sourceUrl;
    QUrl playbackUrl;
};

enum class VideoSourceLoadFailureKind {
    PlaybackUrlResolution,
};

enum class VideoSourceLoadFailureSeverity {
    Error,
};

struct VideoSourceLoadFailure {
    QUrl sourceUrl;
    VideoSourceLoadFailureKind kind = VideoSourceLoadFailureKind::PlaybackUrlResolution;
    QString userMessage;
    QString diagnosticDetail;
    VideoSourceLoadFailureSeverity severity = VideoSourceLoadFailureSeverity::Error;
    bool retryable = false;
};

struct PublishVideoSourceLoadFailureOperation {
    VideoSourceLoadFailure failure;
};

using VideoSourceLoadOperation = std::variant<ClearVideoPlaybackSourceOperation,
    ResetClearedVideoSourceOperation, ResetVideoSourceLoadOperation, ApplyVideoPlaybackUrlOperation,
    PublishVideoSourceLoadFailureOperation>;
using VideoSourceLoadPlan = std::vector<VideoSourceLoadOperation>;

VideoSourceLoadPlan videoSourceLoadClearPlan();
VideoSourceLoadPlan videoSourceLoadStartPlan(const QUrl &sourceUrl);
VideoSourceLoadPlan videoSourceLoadReadyPlan(const QUrl &sourceUrl, const QUrl &playbackUrl);
VideoSourceLoadFailure videoSourceLoadPlaybackUrlResolutionFailure(
    const QUrl &sourceUrl, const QString &diagnosticDetail);
VideoSourceLoadPlan videoSourceLoadFailurePlan(VideoSourceLoadFailure failure);
VideoSourceLoadPlan videoSourceLoadFailurePlan(const QUrl &sourceUrl, const QString &errorString);
}

#endif
