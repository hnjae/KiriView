// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_THUMBNAILGENERATION_H
#define KIRIVIEW_THUMBNAILGENERATION_H

#include "async/imageiojob.h"
#include "session/activenavigationthumbnaildemand.h"

#include <QByteArray>
#include <QImage>
#include <QString>
#include <functional>

class QObject;

namespace KiriView {
enum class ThumbnailGenerationStatus {
    Ready,
    Failed,
};

struct ThumbnailGenerationRequest {
    QByteArray localPathBytes;
    ActiveNavigationThumbnailDemandBucket requestedBucket
        = ActiveNavigationThumbnailDemandBucket::None;
};

struct ThumbnailGenerationResult {
    ThumbnailGenerationStatus status = ThumbnailGenerationStatus::Failed;
    QImage image;
    ActiveNavigationThumbnailDemandBucket requestedBucket
        = ActiveNavigationThumbnailDemandBucket::None;
    QString installedCachePath;
    QString errorString;
};

using ThumbnailGenerationCallback = std::function<void(ThumbnailGenerationResult)>;
using ThumbnailGenerationProvider
    = std::function<ImageIoJob(QObject *, ThumbnailGenerationRequest, ThumbnailGenerationCallback)>;

ThumbnailGenerationProvider defaultThumbnailGenerationProvider();
}

#endif
