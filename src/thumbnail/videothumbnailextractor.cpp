// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "videothumbnailextractor.h"

#include "thumbnail/thumbnailgeneration.h"

#include <QMediaPlayer>
#include <QSize>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoSink>
#include <Qt>
#include <algorithm>
#include <utility>

namespace {
constexpr int extractionTimeoutMsec = 10000;

QSize boundedSize(const QSize &size, int maximumLongEdge)
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
        kiriview::VideoThumbnailExtractionCallback callback, QObject *parent)
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
            [this](const QVideoFrame &frame) { acceptFrame(frame); });
        connect(&m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &errorString) {
                finish(failedExtraction(errorString.isEmpty()
                        ? QStringLiteral("video thumbnail decode failed")
                        : errorString));
            });
        connect(&m_player, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
                if (status == QMediaPlayer::InvalidMedia) {
                    finish(failedExtraction(QStringLiteral("video thumbnail media is invalid")));
                } else if (status == QMediaPlayer::EndOfMedia) {
                    finish(failedExtraction(
                        QStringLiteral("video thumbnail decode produced no frame")));
                }
            });
        connect(&m_timeout, &QTimer::timeout, this, [this]() {
            finish(failedExtraction(QStringLiteral("video thumbnail extraction timed out")));
        });

        m_player.setSource(m_request.sourceUrl);
        m_player.setPosition(0);
        m_player.play();
        m_timeout.start();
    }

    void cancel()
    {
        m_timeout.stop();
        m_player.stop();
        deleteLater();
    }

private:
    void acceptFrame(const QVideoFrame &frame)
    {
        if (!frame.isValid()) {
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
};
}

namespace kiriview {
QImage videoThumbnailImageFromFrameImage(QImage image, int maximumLongEdge, QString *errorString)
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

ImageIoJob startVideoThumbnailExtraction(QObject *receiver, VideoThumbnailExtractionRequest request,
    VideoThumbnailExtractionCallback callback)
{
    auto *jobObject
        = new VideoThumbnailExtractionJob(std::move(request), std::move(callback), receiver);
    ImageIoJob job(jobObject, [](QObject *object) {
        if (auto *extractionJob = dynamic_cast<VideoThumbnailExtractionJob *>(object)) {
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
