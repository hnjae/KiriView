// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_THUMBNAILGENERATION_H
#define KIRIVIEW_THUMBNAILGENERATION_H

#include "async/imageiojob.h"
#include "async/imageworkerscheduler.h"
#include "location/imagelocation.h"
#include "thumbnail/thumbnailbucket.h"
#include "thumbnail/thumbnailcachelookup.h"
#include "thumbnail/thumbnailoriginalidentity.h"
#include "thumbnail/thumbnailsourcekind.h"

#include <QByteArray>
#include <QImage>
#include <QString>
#include <QUrl>
#include <functional>
#include <optional>

class QObject;

namespace kiriview {
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

using ThumbnailGenerationBytesLoader
    = std::function<QByteArray(const ThumbnailGenerationRequest &, QString *)>;
using ThumbnailGenerationOriginalIdentityLoader
    = std::function<std::optional<ThumbnailOriginalIdentity>(
        const ThumbnailGenerationRequest &, QString *)>;
using ThumbnailGenerationImageDecoder = std::function<QImage(QByteArray, int, QString *)>;
using ThumbnailGenerationMaximumLongEdgePolicy
    = std::function<int(ActiveNavigationThumbnailDemandBucket)>;

struct ThumbnailGenerationCacheInstallResult {
    bool success = false;
    ActiveNavigationThumbnailDemandBucket requestedBucket
        = ActiveNavigationThumbnailDemandBucket::None;
    QString installedCachePath;
    QString errorString;
};

using ThumbnailGenerationCacheLookup = std::function<std::optional<ThumbnailCacheLookupResult>(
    const ThumbnailOriginalIdentity &, ActiveNavigationThumbnailDemandBucket)>;
using ThumbnailGenerationCacheInstall = std::function<ThumbnailGenerationCacheInstallResult(
    const ThumbnailOriginalIdentity &, ActiveNavigationThumbnailDemandBucket, const QImage &)>;

struct ThumbnailGenerationCacheRepository {
    ThumbnailGenerationCacheLookup lookup;
    ThumbnailGenerationCacheInstall install;
};

struct ThumbnailGenerationDependencies {
    ThumbnailGenerationBytesLoader bytesLoader;
    ThumbnailGenerationImageDecoder imageDecoder;
    ThumbnailGenerationMaximumLongEdgePolicy maximumLongEdgeForBucket;
    ThumbnailGenerationOriginalIdentityLoader openedCollectionOriginalIdentityLoader;
    ThumbnailGenerationCacheRepository cacheRepository;
};

using ThumbnailGenerationCallback = std::function<void(ThumbnailGenerationResult)>;
using ThumbnailGenerationProvider
    = std::function<ImageIoJob(QObject *, ThumbnailGenerationRequest, ThumbnailGenerationCallback)>;

ThumbnailGenerationDependencies defaultThumbnailGenerationDependencies();

ThumbnailGenerationResult generateThumbnail(
    const ThumbnailGenerationRequest &request, ThumbnailGenerationDependencies dependencies = {});

ThumbnailGenerationProvider defaultThumbnailGenerationProvider(
    ImageWorkerScheduler workerScheduler = {}, ThumbnailGenerationDependencies dependencies = {});
}

#endif
