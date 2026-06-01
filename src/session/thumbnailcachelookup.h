// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_THUMBNAILCACHELOOKUP_H
#define KIRIVIEW_THUMBNAILCACHELOOKUP_H

#include "async/imageiojob.h"
#include "session/activenavigationthumbnaildemand.h"
#include "session/thumbnailoriginalidentity.h"

#include <QByteArray>
#include <QImage>
#include <QString>
#include <functional>

class QObject;

namespace KiriView {
enum class ThumbnailCacheLookupStatus {
    Ready,
    Missing,
    Invalid,
    Failed,
};

struct ThumbnailCacheLookupRequest {
    QByteArray localPathBytes;
    ThumbnailOriginalIdentity originalIdentity;
    ActiveNavigationThumbnailDemandBucket requestedBucket
        = ActiveNavigationThumbnailDemandBucket::None;
};

struct ThumbnailCacheLookupResult {
    ThumbnailCacheLookupStatus status = ThumbnailCacheLookupStatus::Missing;
    QImage image;
    ActiveNavigationThumbnailDemandBucket requestedBucket
        = ActiveNavigationThumbnailDemandBucket::None;
    ActiveNavigationThumbnailDemandBucket sourceBucket
        = ActiveNavigationThumbnailDemandBucket::None;
    QString sourceCachePath;
    QString errorString;
};

using ThumbnailCacheLookupCallback = std::function<void(ThumbnailCacheLookupResult)>;
using ThumbnailCacheLookupProvider = std::function<ImageIoJob(
    QObject *, ThumbnailCacheLookupRequest, ThumbnailCacheLookupCallback)>;

ThumbnailCacheLookupProvider defaultThumbnailCacheLookupProvider();
}

#endif
