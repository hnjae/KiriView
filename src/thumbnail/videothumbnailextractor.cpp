// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "videothumbnailextractor.h"

#include "thumbnail/thumbnailgeneration.h"

#include <QMediaPlayer>
#include <QSize>
#include <QTimer>
#include <QVariant>
#include <QVideoFrame>
#include <QVideoSink>
#include <Qt>
#include <algorithm>
#include <utility>

namespace {
constexpr int extractionTimeoutMsec = 10000;

QSize boundedSize(QSize size, int maximumLongEdge)
{
    if (size.isEmpty() || maximumLongEdge <= 0) {
        return {};
    }

    const int longEdge = std::max(size.width(), size.height());
    if (longEdge <= maximumLongEdge) {
        return size;
    }

    return size.scaled(QSize(maximumLongEdge, maximumLongEdge), Qt::KeepAspectRatio);
}

QImage embeddedImageFromMetadata(const QMediaMetaData& metadata)
{
    QImage cover = metadata.value(QMediaMetaData::CoverArtImage).value<QImage>();
    if (!cover.isNull()) {
        return cover;
    }

    QImage thumbnail = metadata.value(QMediaMetaData::ThumbnailImage).value<QImage>();
    if (!thumbnail.isNull()) {
        return thumbnail;
    }

    return {};
}

kiriview::VideoThumbnailExtractionResult failedExtraction(QString errorString)
{
    return kiriview::VideoThumbnailExtractionResult {
        kiriview::ThumbnailGenerationStatus::Failed,
        {},
        std::move(errorString),
    };
}

class VideoThumbnailExtractionJob final : public QObject
{
public:
    VideoThumbnailExtractionJob(kiriview::VideoThumbnailExtractionRequest request,
        kiriview::VideoThumbnailExtractionCallback callback, QObject* parent)
        : QObject(parent)
        , m_request(std::move(request))
        , m_callback(std::move(callback))
    {
        m_timeout.setSingleShot(true);
        m_timeout.setInterval(extractionTimeoutMsec);
    }

    void setCompletion(kiriview::ImageIoJobCompletion completion)
    {
        m_completion = std::move(completion);
    }

    void start()
    {
        if (m_request.sourceUrl.isEmpty()) {
            finish(failedExtraction(QStringLiteral("video thumbnail source URL is empty")));
            return;
        }
        if (m_request.maximumLongEdge <= 0) {
            finish(failedExtraction(QStringLiteral("video thumbnail requires a size bucket")));
            return;
        }

        m_player.setVideoSink(&m_sink);
        connect(&m_sink, &QVideoSink::videoFrameChanged, this,
            [this](const QVideoFrame& frame) { acceptFrame(frame); });
        connect(&m_player, &QMediaPlayer::metaDataChanged, this,
            [this]() { acceptMetadataIfAvailable(); });
        connect(&m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString& errorString) {
                if (acceptMetadataIfAvailable()) {
                    return;
                }
                finish(failedExtraction(errorString.isEmpty()
                        ? QStringLiteral("video thumbnail decode failed")
                        : errorString));
            });
        connect(&m_player, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) { handleMediaStatus(status); });
        connect(&m_timeout, &QTimer::timeout, this, [this]() {
            if (acceptMetadataIfAvailable()) {
                return;
            }
            finish(failedExtraction(QStringLiteral("video thumbnail extraction timed out")));
        });

        m_player.setSource(m_request.sourceUrl);
        m_player.setPosition(0);
        m_timeout.start();
        if (!acceptMetadataIfAvailable()) {
            handleMediaStatus(m_player.mediaStatus());
        }
    }

    void cancel()
    {
        m_timeout.stop();
        m_player.stop();
        deleteLater();
    }

private:
    void handleMediaStatus(QMediaPlayer::MediaStatus status)
    {
        if (m_finished || acceptMetadataIfAvailable()) {
            return;
        }

        switch (status) {
        case QMediaPlayer::InvalidMedia:
            finish(failedExtraction(QStringLiteral("video thumbnail media is invalid")));
            break;
        case QMediaPlayer::LoadedMedia:
        case QMediaPlayer::BufferedMedia:
            startFrameExtraction();
            break;
        case QMediaPlayer::EndOfMedia:
            if (m_frameExtractionStarted) {
                finish(
                    failedExtraction(QStringLiteral("video thumbnail decode produced no frame")));
            }
            break;
        case QMediaPlayer::NoMedia:
        case QMediaPlayer::LoadingMedia:
        case QMediaPlayer::BufferingMedia:
        case QMediaPlayer::StalledMedia:
            break;
        }
    }

    bool acceptMetadataIfAvailable()
    {
        if (m_finished) {
            return true;
        }

        QString errorString;
        QImage image = kiriview::videoThumbnailImageFromMetadata(
            m_player.metaData(), m_request.maximumLongEdge, &errorString);
        if (image.isNull()) {
            return false;
        }

        finish(kiriview::VideoThumbnailExtractionResult {
            kiriview::ThumbnailGenerationStatus::Ready,
            std::move(image),
            {},
        });
        return true;
    }

    void startFrameExtraction()
    {
        if (m_finished || m_frameExtractionStarted || acceptMetadataIfAvailable()) {
            return;
        }

        m_frameExtractionStarted = true;
        m_player.setPosition(0);
        m_player.play();
    }

    void acceptFrame(const QVideoFrame& frame)
    {
        if (m_finished || !frame.isValid() || acceptMetadataIfAvailable()) {
            return;
        }

        QString errorString;
        QImage image = kiriview::videoThumbnailImageFromFrameImage(
            frame.toImage(), m_request.maximumLongEdge, &errorString);
        if (image.isNull()) {
            finish(failedExtraction(errorString.isEmpty()
                    ? QStringLiteral("video thumbnail frame conversion failed")
                    : std::move(errorString)));
            return;
        }

        finish(kiriview::VideoThumbnailExtractionResult {
            kiriview::ThumbnailGenerationStatus::Ready,
            std::move(image),
            {},
        });
    }

    void finish(kiriview::VideoThumbnailExtractionResult result)
    {
        if (m_finished) {
            return;
        }
        m_finished = true;
        m_timeout.stop();
        m_player.stop();

        kiriview::ImageIoJobCompletion completion = m_completion;
        kiriview::VideoThumbnailExtractionCallback callback = std::move(m_callback);
        completion.claimAndDelete(
            [callback = std::move(callback), result = std::move(result)]() mutable {
                if (callback) {
                    callback(std::move(result));
                }
            });
    }

    kiriview::VideoThumbnailExtractionRequest m_request;
    kiriview::VideoThumbnailExtractionCallback m_callback;
    kiriview::ImageIoJobCompletion m_completion;
    QMediaPlayer m_player;
    QVideoSink m_sink;
    QTimer m_timeout;
    bool m_frameExtractionStarted = false;
    bool m_finished = false;
};
}

namespace kiriview {
QImage videoThumbnailImageFromFrameImage(QImage image, int maximumLongEdge, QString* errorString)
{
    if (image.isNull()) {
        if (errorString != nullptr) {
            *errorString = QStringLiteral("video frame produced no image");
        }
        return {};
    }

    const QSize targetSize = boundedSize(image.size(), maximumLongEdge);
    if (targetSize.isEmpty()) {
        if (errorString != nullptr) {
            *errorString = QStringLiteral("video thumbnail requires a valid size");
        }
        return {};
    }
    if (targetSize == image.size()) {
        return image;
    }

    return image.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QImage videoThumbnailImageFromMetadata(
    const QMediaMetaData& metadata, int maximumLongEdge, QString* errorString)
{
    QImage image = embeddedImageFromMetadata(metadata);
    if (image.isNull()) {
        if (errorString != nullptr) {
            *errorString = QStringLiteral("video metadata produced no embedded image");
        }
        return {};
    }

    return videoThumbnailImageFromFrameImage(std::move(image), maximumLongEdge, errorString);
}

ImageIoJob startVideoThumbnailExtraction(QObject* receiver, VideoThumbnailExtractionRequest request,
    VideoThumbnailExtractionCallback callback)
{
    auto* jobObject
        = new VideoThumbnailExtractionJob(std::move(request), std::move(callback), receiver);
    ImageIoJob job(jobObject, [](QObject* object) {
        if (auto* extractionJob = dynamic_cast<VideoThumbnailExtractionJob*>(object)) {
            extractionJob->cancel();
        } else if (object != nullptr) {
            object->deleteLater();
        }
    });
    jobObject->setCompletion(job.completion());
    jobObject->start();
    return job;
}
}
