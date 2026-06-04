// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_THUMBNAILPREVIEW_H
#define KIRIVIEW_THUMBNAILPREVIEW_H

#include "decoding/imagedecoderequest.h"
#include "rendering/displayimagequality.h"
#include "rendering/staticimage.h"
#include "session/activenavigationthumbnaildemand.h"
#include "session/thumbnailcachelookup.h"

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QString>
#include <QUrl>
#include <optional>

namespace KiriView {
struct XdgThumbnailPreviewRequest {
    QUrl sourceUrl;
    QSize trustedOriginalSize;
    ActiveNavigationThumbnailDemandBucket requestedBucket
        = ActiveNavigationThumbnailDemandBucket::None;
    bool stillImage = true;
};

struct XdgThumbnailPreviewResult {
    ThumbnailCacheLookupStatus status = ThumbnailCacheLookupStatus::Missing;
    QImage image;
    QSize originalSize;
    DisplayImageQuality quality = DisplayImageQuality::ThumbnailPreview;
    ActiveNavigationThumbnailDemandBucket requestedBucket
        = ActiveNavigationThumbnailDemandBucket::None;
    ActiveNavigationThumbnailDemandBucket sourceBucket
        = ActiveNavigationThumbnailDemandBucket::None;
    QString sourceCachePath;
    QString errorString;
};

std::optional<ThumbnailCacheLookupRequest> xdgThumbnailPreviewCacheLookupRequest(
    const XdgThumbnailPreviewRequest &request);
XdgThumbnailPreviewResult xdgThumbnailPreviewResult(
    const XdgThumbnailPreviewRequest &request, ThumbnailCacheLookupResult lookupResult);
std::optional<XdgThumbnailPreviewRequest> xdgThumbnailPreviewRequestForDecodeData(
    const QByteArray &data, const ImageDecodeRequest &request);
std::optional<StaticDisplayImagePayload> xdgThumbnailPreviewDisplayPayload(
    const ImageDecodeRequest &request, XdgThumbnailPreviewResult result);
}

#endif
