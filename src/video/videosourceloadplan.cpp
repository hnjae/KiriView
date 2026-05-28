// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videosourceloadplan.h"

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

VideoSourceLoadPlan videoSourceLoadFailurePlan(const QUrl &sourceUrl, const QString &errorString)
{
    return {
        PublishVideoSourceLoadFailureOperation {
            sourceUrl,
            errorString,
        },
    };
}
}
