// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEOTHUMBNAILEXTRACTOR_H
#define KIRIVIEW_VIDEOTHUMBNAILEXTRACTOR_H

#include "async/imageiojob.h"
#include "session/activenavigationthumbnaildemand.h"

#include <QByteArray>
#include <QImage>
#include <QString>
#include <QUrl>
#include <functional>

class QObject;

namespace kiriview {
enum class ThumbnailGenerationStatus;

struct VideoThumbnailExtractionRequest {
    QByteArray localPathBytes;
    QUrl sourceUrl;
    ActiveNavigationThumbnailDemandBucket requestedBucket
        = ActiveNavigationThumbnailDemandBucket::None;
    int maximumLongEdge = 0;
};

struct VideoThumbnailExtractionResult {
    ThumbnailGenerationStatus status;
    QImage image;
    QString errorString;
};

using VideoThumbnailExtractionCallback = std::function<void(VideoThumbnailExtractionResult)>;
using VideoThumbnailExtractionProvider = std::function<ImageIoJob(
    QObject *, VideoThumbnailExtractionRequest, VideoThumbnailExtractionCallback)>;

QImage videoThumbnailImageFromFrameImage(
    QImage image, int maximumLongEdge, QString *errorString = nullptr);

ImageIoJob startVideoThumbnailExtraction(QObject *receiver, VideoThumbnailExtractionRequest request,
    VideoThumbnailExtractionCallback callback);
}

#endif
