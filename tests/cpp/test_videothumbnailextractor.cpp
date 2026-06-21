// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "thumbnail/videothumbnailextractor.h"

#include <QColor>
#include <QImage>
#include <QObject>
#include <QSize>
#include <QString>
#include <QTest>

class TestVideoThumbnailExtractor : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void framePostProcessingScalesToBucketEdge();
    void framePostProcessingRejectsNullImages();
};

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

QTEST_GUILESS_MAIN(TestVideoThumbnailExtractor)

#include "test_videothumbnailextractor.moc"
