// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/thumbnailpreview.h"

#include <QColor>
#include <QFile>
#include <QImage>
#include <QObject>
#include <QSize>
#include <QString>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
using Status = KiriView::ThumbnailCacheLookupStatus;

KiriView::XdgThumbnailPreviewRequest previewRequest(
    QUrl sourceUrl = QUrl::fromLocalFile(QStringLiteral("/tmp/source.jpg")),
    QSize trustedOriginalSize = QSize(4000, 3000), Bucket bucket = Bucket::XXLarge)
{
    return KiriView::XdgThumbnailPreviewRequest {
        std::move(sourceUrl),
        trustedOriginalSize,
        bucket,
        true,
    };
}

QImage previewImage(const QSize &size)
{
    QImage image(size, QImage::Format_RGBA8888);
    image.fill(QColor(Qt::red));
    return image;
}

KiriView::ThumbnailCacheLookupResult lookupResult(Status status,
    QImage image = previewImage(QSize(1024, 768)), Bucket sourceBucket = Bucket::XXLarge)
{
    return KiriView::ThumbnailCacheLookupResult {
        status,
        std::move(image),
        Bucket::XXLarge,
        sourceBucket,
        QStringLiteral("/cache/source.png"),
        status == Status::Invalid ? QStringLiteral("invalid metadata") : QString(),
    };
}
}

class TestThumbnailPreview : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void buildsLocalStillLookupRequest();
    void rejectsLookupWithoutLocalStillIdentity();
    void acceptsReadyThumbnailWithTrustedOriginalSize();
    void acceptsFallbackCacheBuckets();
    void acceptsExifRotatedProjectionOnlyWithTrustedDimensions();
    void rejectsMissingInvalidFailedAndNullLookupResults();
    void rejectsMissingMismatchedOrOversizedOriginalSize();
};

void TestThumbnailPreview::buildsLocalStillLookupRequest()
{
    const KiriView::XdgThumbnailPreviewRequest request = previewRequest();

    const std::optional<KiriView::ThumbnailCacheLookupRequest> lookup
        = KiriView::xdgThumbnailPreviewCacheLookupRequest(request);

    QVERIFY(lookup.has_value());
    QCOMPARE(lookup->localPathBytes, QFile::encodeName(request.sourceUrl.toLocalFile()));
    QVERIFY(lookup->originalIdentity.isLocalPath());
    QCOMPARE(lookup->originalIdentity.localPathBytes,
        QFile::encodeName(request.sourceUrl.toLocalFile()));
    QCOMPARE(lookup->requestedBucket, Bucket::XXLarge);
}

void TestThumbnailPreview::rejectsLookupWithoutLocalStillIdentity()
{
    QVERIFY(!KiriView::xdgThumbnailPreviewCacheLookupRequest(
        previewRequest(QUrl(QStringLiteral("https://example.test/image.jpg"))))
            .has_value());

    KiriView::XdgThumbnailPreviewRequest animated = previewRequest();
    animated.stillImage = false;
    QVERIFY(!KiriView::xdgThumbnailPreviewCacheLookupRequest(animated).has_value());

    QVERIFY(!KiriView::xdgThumbnailPreviewCacheLookupRequest(
        previewRequest(QUrl::fromLocalFile(QStringLiteral("/tmp/source.jpg")), QSize()))
            .has_value());

    QVERIFY(!KiriView::xdgThumbnailPreviewCacheLookupRequest(
        previewRequest(QUrl::fromLocalFile(QStringLiteral("/tmp/source.jpg")), QSize(4000, 3000),
            Bucket::None))
            .has_value());
}

void TestThumbnailPreview::acceptsReadyThumbnailWithTrustedOriginalSize()
{
    const KiriView::XdgThumbnailPreviewRequest request = previewRequest();

    const KiriView::XdgThumbnailPreviewResult result
        = KiriView::xdgThumbnailPreviewResult(request, lookupResult(Status::Ready));

    QCOMPARE(result.status, Status::Ready);
    QCOMPARE(result.image.size(), QSize(1024, 768));
    QCOMPARE(result.originalSize, QSize(4000, 3000));
    QCOMPARE(result.quality, KiriView::DisplayImageQuality::ThumbnailPreview);
    QCOMPARE(result.requestedBucket, Bucket::XXLarge);
    QCOMPARE(result.sourceBucket, Bucket::XXLarge);
    QCOMPARE(result.sourceCachePath, QStringLiteral("/cache/source.png"));
    QVERIFY(result.errorString.isEmpty());
}

void TestThumbnailPreview::acceptsFallbackCacheBuckets()
{
    const KiriView::XdgThumbnailPreviewRequest request = previewRequest();

    const KiriView::XdgThumbnailPreviewResult xLarge = KiriView::xdgThumbnailPreviewResult(
        request, lookupResult(Status::Ready, previewImage(QSize(512, 384)), Bucket::XLarge));
    const KiriView::XdgThumbnailPreviewResult large = KiriView::xdgThumbnailPreviewResult(
        request, lookupResult(Status::Ready, previewImage(QSize(256, 192)), Bucket::Large));

    QCOMPARE(xLarge.status, Status::Ready);
    QCOMPARE(xLarge.sourceBucket, Bucket::XLarge);
    QCOMPARE(large.status, Status::Ready);
    QCOMPARE(large.sourceBucket, Bucket::Large);
}

void TestThumbnailPreview::acceptsExifRotatedProjectionOnlyWithTrustedDimensions()
{
    const KiriView::ThumbnailCacheLookupResult portraitLookup
        = lookupResult(Status::Ready, previewImage(QSize(300, 400)), Bucket::Large);

    const KiriView::XdgThumbnailPreviewResult trustedRotated = KiriView::xdgThumbnailPreviewResult(
        previewRequest(QUrl::fromLocalFile(QStringLiteral("/tmp/rotated.jpg")), QSize(600, 800)),
        portraitLookup);
    const KiriView::XdgThumbnailPreviewResult unrotatedDimensions
        = KiriView::xdgThumbnailPreviewResult(
            previewRequest(
                QUrl::fromLocalFile(QStringLiteral("/tmp/rotated.jpg")), QSize(800, 600)),
            portraitLookup);

    QCOMPARE(trustedRotated.status, Status::Ready);
    QCOMPARE(unrotatedDimensions.status, Status::Invalid);
}

void TestThumbnailPreview::rejectsMissingInvalidFailedAndNullLookupResults()
{
    const KiriView::XdgThumbnailPreviewRequest request = previewRequest();

    QCOMPARE(KiriView::xdgThumbnailPreviewResult(request, lookupResult(Status::Missing)).status,
        Status::Missing);
    QCOMPARE(KiriView::xdgThumbnailPreviewResult(request, lookupResult(Status::Invalid)).status,
        Status::Invalid);
    QCOMPARE(KiriView::xdgThumbnailPreviewResult(request, lookupResult(Status::Failed)).status,
        Status::Failed);

    KiriView::ThumbnailCacheLookupResult nullReady = lookupResult(Status::Ready);
    nullReady.image = QImage();
    const KiriView::XdgThumbnailPreviewResult result
        = KiriView::xdgThumbnailPreviewResult(request, std::move(nullReady));
    QCOMPARE(result.status, Status::Invalid);
    QVERIFY(result.image.isNull());
}

void TestThumbnailPreview::rejectsMissingMismatchedOrOversizedOriginalSize()
{
    QCOMPARE(KiriView::xdgThumbnailPreviewResult(
                 previewRequest(QUrl::fromLocalFile(QStringLiteral("/tmp/source.jpg")), QSize()),
                 lookupResult(Status::Ready))
                 .status,
        Status::Invalid);

    QCOMPARE(KiriView::xdgThumbnailPreviewResult(
                 previewRequest(), lookupResult(Status::Ready, previewImage(QSize(800, 800))))
                 .status,
        Status::Invalid);

    QCOMPARE(KiriView::xdgThumbnailPreviewResult(
                 previewRequest(), lookupResult(Status::Ready, previewImage(QSize(5000, 3750))))
                 .status,
        Status::Invalid);
}

QTEST_GUILESS_MAIN(TestThumbnailPreview)

#include "test_thumbnailpreview.moc"
