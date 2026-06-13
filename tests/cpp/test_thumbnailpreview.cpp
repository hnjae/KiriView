// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/rawthumbnailpreview.h"
#include "decoding/thumbnailpreview.h"

#include <QBuffer>
#include <QColor>
#include <QFile>
#include <QImage>
#include <QImageWriter>
#include <QObject>
#include <QSize>
#include <QString>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
using Status = kiriview::ThumbnailCacheLookupStatus;

kiriview::XdgThumbnailPreviewRequest previewRequest(
    QUrl sourceUrl = QUrl::fromLocalFile(QStringLiteral("/tmp/source.jpg")),
    QSize trustedOriginalSize = QSize(4000, 3000), Bucket bucket = Bucket::XXLarge)
{
    return kiriview::XdgThumbnailPreviewRequest {
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

kiriview::ThumbnailCacheLookupResult lookupResult(Status status,
    QImage image = previewImage(QSize(1024, 768)), Bucket sourceBucket = Bucket::XXLarge)
{
    return kiriview::ThumbnailCacheLookupResult {
        status,
        std::move(image),
        Bucket::XXLarge,
        sourceBucket,
        QStringLiteral("/cache/source.png"),
        status == Status::Invalid ? QStringLiteral("invalid metadata") : QString(),
    };
}

kiriview::RawEmbeddedThumbnailPreviewResult rawReadyResult(
    QImage image = previewImage(QSize(320, 240)), QSize originalSize = QSize(640, 480))
{
    return kiriview::RawEmbeddedThumbnailPreviewResult {
        kiriview::RawEmbeddedThumbnailPreviewStatus::Ready,
        std::move(image),
        originalSize,
        {},
    };
}

QByteArray encodedPngData()
{
    QImage image(QSize(4, 3), QImage::Format_RGBA8888);
    image.fill(QColor(Qt::green));

    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    QImageWriter writer(&buffer, QByteArrayLiteral("png"));
    if (!writer.write(image)) {
        return {};
    }
    return data;
}

QByteArray rawFixtureData()
{
    QFile file(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR "/../fixtures/raw-cfa-smoke.dng"));
    if (!file.open(QFile::ReadOnly)) {
        return {};
    }
    return file.readAll();
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
    void rawEmbeddedPreviewPayloadUsesPreviewQualityAndOrigin();
    void rawEmbeddedPreviewPayloadRejectsInvalidImageOrOriginalSize();
    void nonRawDataDoesNotExtractRawEmbeddedPreview();
    void rawFixtureEmbeddedPreviewReturnsReadyOrMissingWithoutFailure();
};

void TestThumbnailPreview::buildsLocalStillLookupRequest()
{
    const kiriview::XdgThumbnailPreviewRequest request = previewRequest();

    const std::optional<kiriview::ThumbnailCacheLookupRequest> lookup
        = kiriview::xdgThumbnailPreviewCacheLookupRequest(request);

    QVERIFY(lookup.has_value());
    QCOMPARE(lookup->localPathBytes, QFile::encodeName(request.sourceUrl.toLocalFile()));
    QVERIFY(lookup->originalIdentity.isLocalPath());
    QCOMPARE(lookup->originalIdentity.localPathBytes,
        QFile::encodeName(request.sourceUrl.toLocalFile()));
    QCOMPARE(lookup->requestedBucket, Bucket::XXLarge);
}

void TestThumbnailPreview::rejectsLookupWithoutLocalStillIdentity()
{
    QVERIFY(!kiriview::xdgThumbnailPreviewCacheLookupRequest(
        previewRequest(QUrl(QStringLiteral("https://example.test/image.jpg"))))
            .has_value());

    kiriview::XdgThumbnailPreviewRequest animated = previewRequest();
    animated.stillImage = false;
    QVERIFY(!kiriview::xdgThumbnailPreviewCacheLookupRequest(animated).has_value());

    QVERIFY(!kiriview::xdgThumbnailPreviewCacheLookupRequest(
        previewRequest(QUrl::fromLocalFile(QStringLiteral("/tmp/source.jpg")), QSize()))
            .has_value());

    QVERIFY(!kiriview::xdgThumbnailPreviewCacheLookupRequest(
        previewRequest(QUrl::fromLocalFile(QStringLiteral("/tmp/source.jpg")), QSize(4000, 3000),
            Bucket::None))
            .has_value());
}

void TestThumbnailPreview::acceptsReadyThumbnailWithTrustedOriginalSize()
{
    const kiriview::XdgThumbnailPreviewRequest request = previewRequest();

    const kiriview::XdgThumbnailPreviewResult result
        = kiriview::xdgThumbnailPreviewResult(request, lookupResult(Status::Ready));

    QCOMPARE(result.status, Status::Ready);
    QCOMPARE(result.image.size(), QSize(1024, 768));
    QCOMPARE(result.originalSize, QSize(4000, 3000));
    QCOMPARE(result.quality, kiriview::DisplayImageQuality::ThumbnailPreview);
    QCOMPARE(result.requestedBucket, Bucket::XXLarge);
    QCOMPARE(result.sourceBucket, Bucket::XXLarge);
    QCOMPARE(result.sourceCachePath, QStringLiteral("/cache/source.png"));
    QVERIFY(result.errorString.isEmpty());
}

void TestThumbnailPreview::acceptsFallbackCacheBuckets()
{
    const kiriview::XdgThumbnailPreviewRequest request = previewRequest();

    const kiriview::XdgThumbnailPreviewResult xLarge = kiriview::xdgThumbnailPreviewResult(
        request, lookupResult(Status::Ready, previewImage(QSize(512, 384)), Bucket::XLarge));
    const kiriview::XdgThumbnailPreviewResult large = kiriview::xdgThumbnailPreviewResult(
        request, lookupResult(Status::Ready, previewImage(QSize(256, 192)), Bucket::Large));

    QCOMPARE(xLarge.status, Status::Ready);
    QCOMPARE(xLarge.sourceBucket, Bucket::XLarge);
    QCOMPARE(large.status, Status::Ready);
    QCOMPARE(large.sourceBucket, Bucket::Large);
}

void TestThumbnailPreview::acceptsExifRotatedProjectionOnlyWithTrustedDimensions()
{
    const kiriview::ThumbnailCacheLookupResult portraitLookup
        = lookupResult(Status::Ready, previewImage(QSize(300, 400)), Bucket::Large);

    const kiriview::XdgThumbnailPreviewResult trustedRotated = kiriview::xdgThumbnailPreviewResult(
        previewRequest(QUrl::fromLocalFile(QStringLiteral("/tmp/rotated.jpg")), QSize(600, 800)),
        portraitLookup);
    const kiriview::XdgThumbnailPreviewResult unrotatedDimensions
        = kiriview::xdgThumbnailPreviewResult(
            previewRequest(
                QUrl::fromLocalFile(QStringLiteral("/tmp/rotated.jpg")), QSize(800, 600)),
            portraitLookup);

    QCOMPARE(trustedRotated.status, Status::Ready);
    QCOMPARE(unrotatedDimensions.status, Status::Invalid);
}

void TestThumbnailPreview::rejectsMissingInvalidFailedAndNullLookupResults()
{
    const kiriview::XdgThumbnailPreviewRequest request = previewRequest();

    QCOMPARE(kiriview::xdgThumbnailPreviewResult(request, lookupResult(Status::Missing)).status,
        Status::Missing);
    QCOMPARE(kiriview::xdgThumbnailPreviewResult(request, lookupResult(Status::Invalid)).status,
        Status::Invalid);
    QCOMPARE(kiriview::xdgThumbnailPreviewResult(request, lookupResult(Status::Failed)).status,
        Status::Failed);

    kiriview::ThumbnailCacheLookupResult nullReady = lookupResult(Status::Ready);
    nullReady.image = QImage();
    const kiriview::XdgThumbnailPreviewResult result
        = kiriview::xdgThumbnailPreviewResult(request, std::move(nullReady));
    QCOMPARE(result.status, Status::Invalid);
    QVERIFY(result.image.isNull());
}

void TestThumbnailPreview::rejectsMissingMismatchedOrOversizedOriginalSize()
{
    QCOMPARE(kiriview::xdgThumbnailPreviewResult(
                 previewRequest(QUrl::fromLocalFile(QStringLiteral("/tmp/source.jpg")), QSize()),
                 lookupResult(Status::Ready))
                 .status,
        Status::Invalid);

    QCOMPARE(kiriview::xdgThumbnailPreviewResult(
                 previewRequest(), lookupResult(Status::Ready, previewImage(QSize(800, 800))))
                 .status,
        Status::Invalid);

    QCOMPARE(kiriview::xdgThumbnailPreviewResult(
                 previewRequest(), lookupResult(Status::Ready, previewImage(QSize(5000, 3750))))
                 .status,
        Status::Invalid);
}

void TestThumbnailPreview::rawEmbeddedPreviewPayloadUsesPreviewQualityAndOrigin()
{
    const kiriview::ImageDecodeRequest request = kiriview::ImageDecodeRequest::fromUrl(
        12, QUrl::fromLocalFile(QStringLiteral("/tmp/source.dng")));

    const std::optional<kiriview::StaticDisplayImagePayload> payload
        = kiriview::rawEmbeddedThumbnailPreviewDisplayPayload(request, rawReadyResult());

    QVERIFY(payload.has_value());
    QCOMPARE(payload->quality, kiriview::DisplayImageQuality::ThumbnailPreview);
    QCOMPARE(payload->previewOrigin, kiriview::DisplayImagePreviewOrigin::RawEmbeddedThumbnail);
    QCOMPARE(payload->originalSize, QSize(640, 480));
    QCOMPARE(payload->image.size(), QSize(320, 240));
    QVERIFY(payload->refinementSource == nullptr);
}

void TestThumbnailPreview::rawEmbeddedPreviewPayloadRejectsInvalidImageOrOriginalSize()
{
    const kiriview::ImageDecodeRequest request = kiriview::ImageDecodeRequest::fromUrl(
        13, QUrl::fromLocalFile(QStringLiteral("/tmp/source.dng")));

    QVERIFY(!kiriview::rawEmbeddedThumbnailPreviewDisplayPayload(
        request, rawReadyResult(QImage(), QSize(640, 480)))
            .has_value());
    QVERIFY(!kiriview::rawEmbeddedThumbnailPreviewDisplayPayload(
        request, rawReadyResult(previewImage(QSize(320, 240)), QSize()))
            .has_value());
    QVERIFY(!kiriview::rawEmbeddedThumbnailPreviewDisplayPayload(
        request, rawReadyResult(previewImage(QSize(300, 300)), QSize(640, 480)))
            .has_value());
}

void TestThumbnailPreview::nonRawDataDoesNotExtractRawEmbeddedPreview()
{
    const QByteArray data = encodedPngData();
    QVERIFY(!data.isEmpty());
    const kiriview::ImageDecodeRequest request = kiriview::ImageDecodeRequest::fromUrl(
        14, QUrl::fromLocalFile(QStringLiteral("/tmp/source.png")));

    const kiriview::RawEmbeddedThumbnailPreviewResult result
        = kiriview::rawEmbeddedThumbnailPreviewResult(data, request);

    QCOMPARE(result.status, kiriview::RawEmbeddedThumbnailPreviewStatus::Missing);
    QVERIFY(result.image.isNull());
}

void TestThumbnailPreview::rawFixtureEmbeddedPreviewReturnsReadyOrMissingWithoutFailure()
{
    const QByteArray data = rawFixtureData();
    QVERIFY(!data.isEmpty());
    const kiriview::ImageDecodeRequest request = kiriview::ImageDecodeRequest::fromUrl(
        15, QUrl::fromLocalFile(QStringLiteral("/tmp/raw-cfa-smoke.dng")));

    const kiriview::RawEmbeddedThumbnailPreviewResult result
        = kiriview::rawEmbeddedThumbnailPreviewResult(data, request);

    QVERIFY(result.status == kiriview::RawEmbeddedThumbnailPreviewStatus::Ready
        || result.status == kiriview::RawEmbeddedThumbnailPreviewStatus::Missing);
    if (result.status == kiriview::RawEmbeddedThumbnailPreviewStatus::Ready) {
        QCOMPARE(result.originalSize, QSize(32, 32));
        QVERIFY(kiriview::rawEmbeddedThumbnailPreviewDisplayPayload(request, result).has_value());
    }
}

QTEST_GUILESS_MAIN(TestThumbnailPreview)

#include "test_thumbnailpreview.moc"
