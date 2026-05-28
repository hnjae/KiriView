// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEOSOURCELOADPLAN_H
#define KIRIVIEW_VIDEOSOURCELOADPLAN_H

#include <QString>
#include <QUrl>
#include <variant>
#include <vector>

namespace KiriView {
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

struct PublishVideoSourceLoadFailureOperation {
    QUrl sourceUrl;
    QString errorString;
};

using VideoSourceLoadOperation = std::variant<ClearVideoPlaybackSourceOperation,
    ResetClearedVideoSourceOperation, ResetVideoSourceLoadOperation, ApplyVideoPlaybackUrlOperation,
    PublishVideoSourceLoadFailureOperation>;
using VideoSourceLoadPlan = std::vector<VideoSourceLoadOperation>;

VideoSourceLoadPlan videoSourceLoadClearPlan();
VideoSourceLoadPlan videoSourceLoadStartPlan(const QUrl &sourceUrl);
VideoSourceLoadPlan videoSourceLoadReadyPlan(const QUrl &sourceUrl, const QUrl &playbackUrl);
VideoSourceLoadPlan videoSourceLoadFailurePlan(const QUrl &sourceUrl, const QString &errorString);
}

#endif
