// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "thumbnail/videothumbnailextractor.h"

#include <QColor>
#include <QImage>
#include <QMediaMetaData>
#include <QObject>
#include <QSize>
#include <QString>
#include <QTest>
#include <QVector>

namespace kiriview {
QVector<qint64> videoThumbnailCandidatePositions(qint64 durationMsec);
bool videoThumbnailFrameIsInteresting(const QImage& image);
}

class TestVideoThumbnailExtractor : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void interestingFrameRejectsSolidImage();
    void interestingFrameAcceptsHighVarianceImage();
    void interestingFrameRejectsNullImage();
    void candidatePositionsUseTotemOrder();
    void candidatePositionsRejectNonPositiveDuration();
    void framePostProcessingScalesToBucketEdge();
    void framePostProcessingRejectsNullImages();
    void metadataPostProcessingPrefersCoverArtImage();
    void metadataPostProcessingUsesThumbnailImageFallback();
    void metadataPostProcessingRejectsMissingEmbeddedImage();
};

void TestVideoThumbnailExtractor::interestingFrameRejectsSolidImage()
{
    QImage frame(QSize(16, 16), QImage::Format_RGB32);
    frame.fill(QColor(Qt::black));

    QVERIFY(!kiriview::videoThumbnailFrameIsInteresting(frame));
}

void TestVideoThumbnailExtractor::interestingFrameAcceptsHighVarianceImage()
{
    QImage frame(QSize(16, 16), QImage::Format_RGB32);
    for (int y = 0; y < frame.height(); ++y) {
        for (int x = 0; x < frame.width(); ++x) {
            frame.setPixelColor(
                x, y, x < frame.width() / 2 ? QColor(Qt::black) : QColor(Qt::white));
        }
    }

    QVERIFY(kiriview::videoThumbnailFrameIsInteresting(frame));
}

void TestVideoThumbnailExtractor::interestingFrameRejectsNullImage()
{
    QVERIFY(!kiriview::videoThumbnailFrameIsInteresting({}));
}

void TestVideoThumbnailExtractor::candidatePositionsUseTotemOrder()
{
    const QVector<qint64> positions = kiriview::videoThumbnailCandidatePositions(90000);

    QCOMPARE(positions, QVector<qint64>({ 30000, 60000, 9000, 81000, 45000 }));
}

void TestVideoThumbnailExtractor::candidatePositionsRejectNonPositiveDuration()
{
    QVERIFY(kiriview::videoThumbnailCandidatePositions(0).isEmpty());
    QVERIFY(kiriview::videoThumbnailCandidatePositions(-1).isEmpty());
}

void TestVideoThumbnailExtractor::framePostProcessingScalesToBucketEdge()
{
    QImage frame(QSize(400, 200), QImage::Format_RGB32);
    frame.fill(QColor(Qt::green));
    QString errorString;

    const QImage thumbnail
        = kiriview::videoThumbnailImageFromFrameImage(std::move(frame), 128, &errorString);

    QCOMPARE(thumbnail.size(), QSize(128, 64));
    QCOMPARE(thumbnail.format(), QImage::Format_RGB32);
    QVERIFY(errorString.isEmpty());
}

void TestVideoThumbnailExtractor::framePostProcessingRejectsNullImages()
{
    QString errorString;

    const QImage thumbnail = kiriview::videoThumbnailImageFromFrameImage({}, 128, &errorString);

    QVERIFY(thumbnail.isNull());
    QCOMPARE(errorString, QStringLiteral("video frame produced no image"));
}

void TestVideoThumbnailExtractor::metadataPostProcessingPrefersCoverArtImage()
{
    QImage cover(QSize(300, 150), QImage::Format_RGB32);
    cover.fill(QColor(Qt::red));
    QImage thumbnailFallback(QSize(40, 20), QImage::Format_RGB32);
    thumbnailFallback.fill(QColor(Qt::blue));
    QMediaMetaData metadata;
    metadata[QMediaMetaData::CoverArtImage] = cover;
    metadata[QMediaMetaData::ThumbnailImage] = thumbnailFallback;
    QString errorString;

    const QImage thumbnail = kiriview::videoThumbnailImageFromMetadata(metadata, 128, &errorString);

    QCOMPARE(thumbnail.size(), QSize(128, 64));
    QCOMPARE(thumbnail.pixelColor(0, 0), QColor(Qt::red));
    QVERIFY(errorString.isEmpty());
}

void TestVideoThumbnailExtractor::metadataPostProcessingUsesThumbnailImageFallback()
{
    QImage embeddedThumbnail(QSize(90, 60), QImage::Format_RGB32);
    embeddedThumbnail.fill(QColor(Qt::blue));
    QMediaMetaData metadata;
    metadata[QMediaMetaData::ThumbnailImage] = embeddedThumbnail;
    QString errorString;

    const QImage thumbnail = kiriview::videoThumbnailImageFromMetadata(metadata, 45, &errorString);

    QCOMPARE(thumbnail.size(), QSize(45, 30));
    QCOMPARE(thumbnail.pixelColor(0, 0), QColor(Qt::blue));
    QVERIFY(errorString.isEmpty());
}

void TestVideoThumbnailExtractor::metadataPostProcessingRejectsMissingEmbeddedImage()
{
    QMediaMetaData metadata;
    QString errorString;

    const QImage thumbnail = kiriview::videoThumbnailImageFromMetadata(metadata, 128, &errorString);

    QVERIFY(thumbnail.isNull());
    QCOMPARE(errorString, QStringLiteral("video metadata produced no embedded image"));
}

QTEST_GUILESS_MAIN(TestVideoThumbnailExtractor)

#include "test_videothumbnailextractor.moc"
