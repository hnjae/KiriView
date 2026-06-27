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
constexpr double boringFrameVarianceThreshold = 256.0;

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
        connect(&m_player, &QMediaPlayer::positionChanged, this,
            [this](qint64 position) { handlePositionChanged(position); });
        connect(&m_player, &QMediaPlayer::durationChanged, this,
            [this](qint64) { maybeStartFrameExtractionFromReadyMedia(); });
        connect(&m_player, &QMediaPlayer::seekableChanged, this,
            [this](bool) { maybeStartFrameExtractionFromReadyMedia(); });
        connect(&m_timeout, &QTimer::timeout, this, [this]() { handleExtractionTimeout(); });

        m_player.setSource(m_request.sourceUrl);
        m_timeout.start();
        if (!acceptMetadataIfAvailable()) {
            handleMediaStatus(m_player.mediaStatus());
        }
    }

    void cancel()
    {
        m_finished = true;
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
            maybeStartFrameExtractionFromReadyMedia();
            break;
        case QMediaPlayer::EndOfMedia:
            handleEndOfMediaDuringFrameExtraction();
            break;
        case QMediaPlayer::NoMedia:
        case QMediaPlayer::LoadingMedia:
        case QMediaPlayer::BufferingMedia:
        case QMediaPlayer::StalledMedia:
            break;
        }
    }

    void handlePositionChanged(qint64)
    {
        if (m_finished || acceptMetadataIfAvailable() || !m_awaitingCandidatePosition) {
            return;
        }

        armCandidateFrameAcceptance();
    }

    void maybeStartFrameExtractionFromReadyMedia()
    {
        if (m_finished || m_frameExtractionStarted || acceptMetadataIfAvailable()) {
            return;
        }

        switch (m_player.mediaStatus()) {
        case QMediaPlayer::LoadedMedia:
        case QMediaPlayer::BufferedMedia:
            startFrameExtraction();
            break;
        case QMediaPlayer::NoMedia:
        case QMediaPlayer::LoadingMedia:
        case QMediaPlayer::BufferingMedia:
        case QMediaPlayer::StalledMedia:
        case QMediaPlayer::EndOfMedia:
        case QMediaPlayer::InvalidMedia:
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
        if (m_player.duration() > 0 && m_player.isSeekable()) {
            m_candidatePositions = kiriview::videoThumbnailCandidatePositions(m_player.duration());
            if (!m_candidatePositions.isEmpty()) {
                seekToNextCandidate();
                return;
            }
        }

        startFirstFrameFallback();
    }

    void startFirstFrameFallback()
    {
        if (m_finished || acceptMetadataIfAvailable()) {
            return;
        }

        m_firstFrameFallback = true;
        m_awaitingCandidatePosition = false;
        m_awaitingCandidateFrame = false;
        m_candidatePositions.clear();
        m_player.setPosition(0);
        m_player.play();
    }

    void seekToNextCandidate()
    {
        if (m_finished || acceptMetadataIfAvailable()) {
            return;
        }

        if (m_candidateIndex >= m_candidatePositions.size()) {
            finishCandidateExtraction();
            return;
        }

        m_awaitingCandidatePosition = true;
        m_awaitingCandidateFrame = false;
        m_player.pause();
        m_player.setPosition(m_candidatePositions.at(m_candidateIndex));
        if (m_player.position() == m_candidatePositions.at(m_candidateIndex)) {
            armCandidateFrameAcceptance();
        }
        m_player.play();
    }

    void armCandidateFrameAcceptance()
    {
        if (m_finished || m_firstFrameFallback || m_candidateIndex >= m_candidatePositions.size()) {
            return;
        }

        m_awaitingCandidatePosition = false;
        m_awaitingCandidateFrame = true;
    }

    void acceptFrame(const QVideoFrame& frame)
    {
        if (m_finished || !frame.isValid() || acceptMetadataIfAvailable()) {
            return;
        }

        if (!m_firstFrameFallback && !m_awaitingCandidateFrame) {
            return;
        }

        QImage frameImage = frame.toImage();
        if (!m_firstFrameFallback) {
            acceptCandidateFrame(std::move(frameImage));
            return;
        }

        finishReadyFromFrameImage(
            std::move(frameImage), QStringLiteral("video thumbnail frame conversion failed"));
    }

    void acceptCandidateFrame(QImage frameImage)
    {
        if (m_finished || acceptMetadataIfAvailable()) {
            return;
        }

        m_awaitingCandidateFrame = false;
        m_player.pause();
        if (frameImage.isNull()) {
            advanceToNextCandidate();
            return;
        }

        if (kiriview::videoThumbnailFrameIsInteresting(frameImage)) {
            finishReadyFromFrameImage(std::move(frameImage),
                QStringLiteral("video thumbnail candidate conversion failed"));
            return;
        }

        m_lastCapturedCandidate = std::move(frameImage);
        advanceToNextCandidate();
    }

    void advanceToNextCandidate()
    {
        ++m_candidateIndex;
        seekToNextCandidate();
    }

    void finishCandidateExtraction()
    {
        if (m_finished) {
            return;
        }

        if (m_lastCapturedCandidate.isNull()) {
            finish(failedExtraction(
                QStringLiteral("video thumbnail decode produced no usable candidate frame")));
            return;
        }

        finishReadyFromFrameImage(std::move(m_lastCapturedCandidate),
            QStringLiteral("video thumbnail candidate conversion failed"));
    }

    void handleEndOfMediaDuringFrameExtraction()
    {
        if (m_finished || !m_frameExtractionStarted) {
            return;
        }

        if (m_firstFrameFallback) {
            finish(failedExtraction(QStringLiteral("video thumbnail decode produced no frame")));
            return;
        }

        if (m_awaitingCandidatePosition || m_awaitingCandidateFrame) {
            m_awaitingCandidatePosition = false;
            m_awaitingCandidateFrame = false;
            advanceToNextCandidate();
        }
    }

    void handleExtractionTimeout()
    {
        if (m_finished || acceptMetadataIfAvailable()) {
            return;
        }

        if (!m_firstFrameFallback && !m_lastCapturedCandidate.isNull()) {
            finishReadyFromFrameImage(std::move(m_lastCapturedCandidate),
                QStringLiteral("video thumbnail candidate conversion failed"));
            return;
        }

        finish(failedExtraction(QStringLiteral("video thumbnail extraction timed out")));
    }

    void finishReadyFromFrameImage(QImage frameImage, QString fallbackErrorString)
    {
        QString errorString;
        QImage image = kiriview::videoThumbnailImageFromFrameImage(
            std::move(frameImage), m_request.maximumLongEdge, &errorString);
        if (image.isNull()) {
            finish(failedExtraction(
                errorString.isEmpty() ? std::move(fallbackErrorString) : std::move(errorString)));
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
    QVector<qint64> m_candidatePositions;
    qsizetype m_candidateIndex = 0;
    QImage m_lastCapturedCandidate;
    bool m_frameExtractionStarted = false;
    bool m_awaitingCandidatePosition = false;
    bool m_awaitingCandidateFrame = false;
    bool m_firstFrameFallback = false;
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

QVector<qint64> videoThumbnailCandidatePositions(qint64 durationMsec)
{
    if (durationMsec <= 0) {
        return {};
    }

    QVector<qint64> positions;
    const auto appendPosition = [&positions, durationMsec](qint64 numerator, qint64 denominator) {
        const qint64 position = std::clamp((durationMsec / denominator) * numerator
                + ((durationMsec % denominator) * numerator) / denominator,
            qint64(0), durationMsec);
        if (!positions.contains(position)) {
            positions.append(position);
        }
    };

    appendPosition(1, 3);
    appendPosition(2, 3);
    appendPosition(1, 10);
    appendPosition(9, 10);
    appendPosition(1, 2);
    return positions;
}

bool videoThumbnailFrameIsInteresting(const QImage& image)
{
    if (image.isNull() || image.size().isEmpty()) {
        return false;
    }

    const QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);
    if (rgbImage.isNull() || rgbImage.size().isEmpty()) {
        return false;
    }

    const qsizetype bytesPerRow = static_cast<qsizetype>(rgbImage.width()) * 3;
    const qsizetype byteCount = bytesPerRow * rgbImage.height();
    if (byteCount <= 1) {
        return false;
    }

    double count = 0.0;
    double mean = 0.0;
    double sumOfSquaredDifferences = 0.0;
    for (int y = 0; y < rgbImage.height(); ++y) {
        const uchar* line = rgbImage.constScanLine(y);
        for (qsizetype x = 0; x < bytesPerRow; ++x) {
            count += 1.0;
            const double value = line[x];
            const double delta = value - mean;
            mean += delta / count;
            const double updatedDelta = value - mean;
            sumOfSquaredDifferences += delta * updatedDelta;
        }
    }

    const double sampleVariance = sumOfSquaredDifferences / (count - 1.0);
    return sampleVariance > boringFrameVarianceThreshold;
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
