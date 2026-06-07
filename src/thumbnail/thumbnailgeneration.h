// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_THUMBNAILGENERATION_H
#define KIRIVIEW_THUMBNAILGENERATION_H

#include "async/imageiojob.h"
#include "async/imageworkerscheduler.h"
#include "location/imagelocation.h"
#include "thumbnail/thumbnailbucket.h"
#include "thumbnail/thumbnailoriginalidentity.h"
#include "thumbnail/thumbnailsourcekind.h"

#include <QByteArray>
#include <QImage>
#include <QString>
#include <QUrl>
#include <functional>

class QObject;

namespace KiriView {
enum class ThumbnailGenerationStatus {
    Ready,
    Failed,
};

struct ThumbnailGenerationRequest {
    QByteArray localPathBytes;
    ThumbnailOriginalIdentity originalIdentity;
    OpenedCollectionScopeLocation openedCollectionScope;
    QUrl sourceUrl;
    QString sourceLabel;
    ThumbnailSourceKind sourceKind = ThumbnailSourceKind::DirectImage;
    ActiveNavigationThumbnailDemandBucket requestedBucket
        = ActiveNavigationThumbnailDemandBucket::None;
    bool cacheInstallEnabled = true;
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

ThumbnailGenerationProvider defaultThumbnailGenerationProvider(
    ImageWorkerScheduler workerScheduler = {});
}

#endif
