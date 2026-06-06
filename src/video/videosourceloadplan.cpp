// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videosourceloadplan.h"

#include "localization/imageerrortext.h"

#include <utility>

namespace KiriView {
VideoSourceLoadPlan videoSourceLoadClearPlan()
{
    return {
        ClearVideoPlaybackSourceOperation {},
        ResetClearedVideoSourceOperation {},
    };
}

VideoSourceLoadPlan videoSourceLoadStartPlan(const QUrl &sourceUrl)
{
    return {
        ClearVideoPlaybackSourceOperation {},
        ResetVideoSourceLoadOperation { sourceUrl },
    };
}

VideoSourceLoadPlan videoSourceLoadReadyPlan(const QUrl &sourceUrl, const QUrl &playbackUrl)
{
    return {
        ApplyVideoPlaybackUrlOperation {
            sourceUrl,
            playbackUrl,
        },
    };
}

VideoSourceLoadFailure videoSourceLoadPlaybackUrlResolutionFailure(
    const QUrl &sourceUrl, const QString &diagnosticDetail)
{
    return VideoSourceLoadFailure {
        sourceUrl,
        VideoSourceLoadFailureKind::PlaybackUrlResolution,
        imageErrorText(ImageErrorTextId::OpenVideo),
        diagnosticDetail,
        VideoSourceLoadFailureSeverity::Error,
        false,
    };
}

VideoSourceLoadPlan videoSourceLoadFailurePlan(VideoSourceLoadFailure failure)
{
    return {
        PublishVideoSourceLoadFailureOperation {
            std::move(failure),
        },
    };
}

VideoSourceLoadPlan videoSourceLoadFailurePlan(const QUrl &sourceUrl, const QString &errorString)
{
    return videoSourceLoadFailurePlan(
        videoSourceLoadPlaybackUrlResolutionFailure(sourceUrl, errorString));
}
}
