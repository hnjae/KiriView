// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepagesurfacecontroller.h"

#include "decoding/kiriimagedecoder.h"
#include "image_test_support.h"
#include "rendering/heiftilesource.h"
#include "rendering/imagerendering.h"
#include "rendering/qimagereadertilesource.h"

#include <libheif/heif.h>

#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

class TestImagePageSurfaceController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void providerEntriesAreReleasedOnSupersessionAndClear();
    void visibleProjectionPinsAndPrioritizesProviderEntry();
    void shadowThumbnailPreviewEntryIsReleasedOnDecodedReplacement();
    void rawShadowThumbnailPreviewEntryIsReleasedOnDecodedReplacement();
    void qtRasterFirstDisplayRefinesToProviderBucket();
    void qtRasterRefinementCompletionIsRejectedAfterSourceReplacement();
    void exactQtRasterCurrentImageDoesNotRequestRefinement();
    void heifFirstDisplayRefinesToProviderBucket();
    void heifRefinementCompletionIsRejectedAfterSourceReplacement();
    void rawFirstDisplayRefinesToProviderBucket();
    void rawRefinementCompletionIsRejectedAfterSourceReplacement();
};

namespace {
constexpr qsizetype testByteBudget = 1024 * 1024;

KiriView::ImageCacheBudgets cacheBudgets(qsizetype displayImageBudget = testByteBudget)
{
    return KiriView::ImageCacheBudgets {
        testByteBudget,
        testByteBudget,
        testByteBudget,
        displayImageBudget,
    };
}

KiriView::ImageDocumentRenderContext renderContext()
{
    return KiriView::ImageDocumentRenderContext {
        1.0,
        KiriView::fallbackTextureSizeMax,
    };
}

KiriView::ImagePresentationRenderProjection visibleProjection(const QSizeF &displaySize)
{
    KiriView::ImagePresentationRenderProjection projection;
    projection.visible = true;
    projection.displaySize = displaySize;
    projection.visibleItemRect = QRectF(0.0, 0.0, displaySize.width(), displaySize.height());
    projection.maximumTextureSize = KiriView::fallbackTextureSizeMax;
    return projection;
}

KiriView::ImagePresentationRenderProjection visibleProjection()
{
    return visibleProjection(QSizeF(8.0, 4.0));
}

KiriView::StaticDisplayImagePayload displayPayload(const QSize &size)
{
    return KiriView::TestSupport::staticDisplayTestImagePayload(
        KiriView::TestSupport::testImage(size));
}

QString entryId(const KiriView::ImageDisplaySourceSlot &slot)
{
    return slot.providerUrl.path().mid(1);
}

QByteArray encodedPng(const QImage &image)
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return data;
}

heif_error appendHeifBytes(heif_context *, const void *data, std::size_t size, void *userdata)
{
    QByteArray *output = static_cast<QByteArray *>(userdata);
    output->append(static_cast<const char *>(data), static_cast<qsizetype>(size));
    return heif_error_success;
}

bool heifOk(const heif_error &error) { return error.code == heif_error_Ok; }

QByteArray encodedHeifStill(const QSize &size)
{
    if (size.isEmpty()) {
        return {};
    }

    std::unique_ptr<heif_context, decltype(&heif_context_free)> context(
        heif_context_alloc(), heif_context_free);
    if (context == nullptr) {
        return {};
    }

    heif_image *rawImage = nullptr;
    if (!heifOk(heif_image_create(size.width(), size.height(), heif_colorspace_RGB,
            heif_chroma_interleaved_RGB, &rawImage))) {
        return {};
    }
    std::unique_ptr<heif_image, decltype(&heif_image_release)> image(rawImage, heif_image_release);

    if (!heifOk(heif_image_add_plane(
            image.get(), heif_channel_interleaved, size.width(), size.height(), 8))) {
        return {};
    }

    std::size_t stride = 0;
    std::uint8_t *pixels = heif_image_get_plane2(image.get(), heif_channel_interleaved, &stride);
    if (pixels == nullptr) {
        return {};
    }

    for (int y = 0; y < size.height(); ++y) {
        for (int x = 0; x < size.width(); ++x) {
            std::uint8_t *pixel
                = pixels + static_cast<std::size_t>(y) * stride + static_cast<std::size_t>(x) * 3;
            pixel[0] = static_cast<std::uint8_t>((x * 13) % 255);
            pixel[1] = static_cast<std::uint8_t>((y * 17) % 255);
            pixel[2] = static_cast<std::uint8_t>(((x + y) * 19) % 255);
        }
    }

    heif_encoder *rawEncoder = nullptr;
    if (!heifOk(heif_context_get_encoder_for_format(
            context.get(), heif_compression_AV1, &rawEncoder))) {
        return {};
    }
    std::unique_ptr<heif_encoder, decltype(&heif_encoder_release)> encoder(
        rawEncoder, heif_encoder_release);
    if (!heifOk(heif_encoder_set_lossy_quality(encoder.get(), 90))) {
        return {};
    }

    heif_image_handle *rawHandle = nullptr;
    if (!heifOk(heif_context_encode_image(
            context.get(), image.get(), encoder.get(), nullptr, &rawHandle))) {
        return {};
    }
    std::unique_ptr<heif_image_handle, decltype(&heif_image_handle_release)> handle(
        rawHandle, heif_image_handle_release);
    heif_context_set_primary_image(context.get(), handle.get());

    QByteArray data;
    heif_writer writer {};
    writer.writer_api_version = 1;
    writer.write = appendHeifBytes;
    if (!heifOk(heif_context_write(context.get(), &writer, &data))) {
        return {};
    }

    return data;
}

KiriView::StaticDisplayImagePayload qtRasterPayload(const QSize &originalSize,
    const QSize &rasterSize, const QString &sourceIdentity,
    KiriView::DisplayImageQuality quality = KiriView::DisplayImageQuality::FirstDisplay)
{
    QString errorString;
    std::shared_ptr<KiriView::QImageReaderTileSource> source
        = KiriView::QImageReaderTileSource::open(
            encodedPng(KiriView::TestSupport::testImage(originalSize)), QByteArrayLiteral("png"),
            &errorString);
    Q_ASSERT(source != nullptr);

    return KiriView::StaticDisplayImagePayload {
        sourceIdentity,
        source->imageReaderTransform(),
        originalSize,
        KiriView::TestSupport::testImage(rasterSize),
        quality,
        KiriView::imagePixelsPerSourcePixel(originalSize, rasterSize),
        {},
        std::move(source),
    };
}

std::optional<KiriView::StaticDisplayImagePayload> heifPayload(const QSize &originalSize,
    const QSize &rasterSize, const QString &sourceIdentity,
    KiriView::DisplayImageQuality quality = KiriView::DisplayImageQuality::FirstDisplay)
{
    QString errorString;
    std::shared_ptr<KiriView::ImageTileSource> source
        = KiriView::openHeifTileSource(encodedHeifStill(originalSize), &errorString);
    if (source == nullptr) {
        return std::nullopt;
    }

    return KiriView::StaticDisplayImagePayload {
        sourceIdentity,
        {},
        originalSize,
        KiriView::TestSupport::testImage(rasterSize),
        quality,
        KiriView::imagePixelsPerSourcePixel(originalSize, rasterSize),
        {},
        std::move(source),
    };
}

QByteArray rawFixtureData()
{
    QFile file(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR "/../fixtures/raw-cfa-smoke.dng"));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    return file.readAll();
}

std::optional<KiriView::StaticDisplayImagePayload> rawPayload(const QSize &rasterSize,
    const QString &sourceIdentity,
    KiriView::DisplayImageQuality quality = KiriView::DisplayImageQuality::FirstDisplay)
{
    const QByteArray data = rawFixtureData();
    if (data.isEmpty()) {
        return std::nullopt;
    }

    const KiriView::ImageDecodeRequest request = KiriView::ImageDecodeRequest::fromUrl(71,
        QUrl::fromLocalFile(
            QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR "/../fixtures/raw-cfa-smoke.dng")));
    const KiriView::DecodedImageResult result = KiriView::decodeImageData(data, request);
    const auto *decoded = KiriView::decodedImageResultImageAs<KiriView::StaticDecodedImage>(result);
    if (decoded == nullptr || decoded->displayImage.refinementSource == nullptr) {
        return std::nullopt;
    }

    KiriView::StaticDisplayImagePayload payload = decoded->displayImage;
    payload.sourceIdentity = sourceIdentity;
    payload.image = KiriView::displayReadyImage(
        payload.image.scaled(rasterSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    payload.quality = quality;
    payload.displayPixelsPerSourcePixel
        = KiriView::imagePixelsPerSourcePixel(payload.originalSize, payload.image.size());
    payload.previewOrigin = KiriView::DisplayImagePreviewOrigin::None;
    return payload;
}
}

void TestImagePageSurfaceController::providerEntriesAreReleasedOnSupersessionAndClear()
{
    auto store = std::make_shared<KiriView::DisplayImageStore>(testByteBudget);
    KiriView::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);

    controller.setStaticDisplayImage(displayPayload(QSize(8, 4)), false, renderContext());
    const KiriView::ImageDisplaySourceSlot first = controller.snapshot().displaySource;
    QVERIFY(!first.providerUrl.isEmpty());
    QVERIFY(store->entry(entryId(first)).has_value());

    controller.setStaticDisplayImage(displayPayload(QSize(6, 4)), false, renderContext());
    const KiriView::ImageDisplaySourceSlot second = controller.snapshot().displaySource;
    QVERIFY(!second.providerUrl.isEmpty());
    QVERIFY(first.providerUrl != second.providerUrl);
    QCOMPARE(second.revision, quint64(2));
    QVERIFY(!store->entry(entryId(first)).has_value());
    QVERIFY(store->entry(entryId(second)).has_value());

    controller.clearImage();
    QVERIFY(!store->entry(entryId(second)).has_value());
    QCOMPARE(
        controller.snapshot().displaySource.status, KiriView::ImageDisplaySourceStatus::Missing);
}

void TestImagePageSurfaceController::visibleProjectionPinsAndPrioritizesProviderEntry()
{
    auto store = std::make_shared<KiriView::DisplayImageStore>(128);
    KiriView::ImagePageSurfaceController controller(this, {}, cacheBudgets(128), store);

    controller.setStaticDisplayImage(displayPayload(QSize(8, 4)), false, renderContext());
    const KiriView::ImageDisplaySourceSlot slot = controller.snapshot().displaySource;
    const QString id = entryId(slot);
    QVERIFY(store->entry(id).has_value());
    QVERIFY(!slot.loadAcknowledgmentRequired);

    controller.scheduleVisibleTileDecode(visibleProjection());
    std::optional<KiriView::DisplayImageStoreEntry> stored = store->entry(id);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->priority, KiriView::DisplayImageRetentionPriority::Visible);

    const QString background = store->insert(KiriView::DisplayImageEntry {
        KiriView::TestSupport::testImage(QSize(8, 4)),
        QSize(8, 4),
        QSize(8, 4),
        QStringLiteral("background"),
        KiriView::DisplayedPageRole::Primary,
        KiriView::DisplayImageQuality::Exact,
        KiriView::DisplayImageRetentionPriority::Background,
        1,
        QStringLiteral("background"),
    });
    Q_UNUSED(background);

    QVERIFY(store->entry(id).has_value());

    KiriView::ImagePresentationRenderProjection hidden;
    controller.scheduleVisibleTileDecode(hidden);
    stored = store->entry(id);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->priority, KiriView::DisplayImageRetentionPriority::Nearby);
}

void TestImagePageSurfaceController::shadowThumbnailPreviewEntryIsReleasedOnDecodedReplacement()
{
    auto store = std::make_shared<KiriView::DisplayImageStore>(testByteBudget);
    KiriView::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);
    KiriView::StaticDisplayImagePayload preview = displayPayload(QSize(80, 60));
    preview.image = KiriView::TestSupport::testImage(QSize(40, 30));
    preview.quality = KiriView::DisplayImageQuality::ThumbnailPreview;
    preview.previewOrigin = KiriView::DisplayImagePreviewOrigin::XdgThumbnail;

    const QString previewId = controller.publishShadowDisplayImage(std::move(preview));

    QVERIFY(!previewId.isEmpty());
    QCOMPARE(
        controller.snapshot().displaySource.status, KiriView::ImageDisplaySourceStatus::Missing);
    std::optional<KiriView::DisplayImageStoreEntry> storedPreview = store->entry(previewId);
    QVERIFY(storedPreview.has_value());
    QCOMPARE(storedPreview->quality, KiriView::DisplayImageQuality::ThumbnailPreview);
    QCOMPARE(storedPreview->previewOrigin, KiriView::DisplayImagePreviewOrigin::XdgThumbnail);
    QCOMPARE(storedPreview->originalSize, QSize(80, 60));
    QCOMPARE(storedPreview->rasterSize, QSize(40, 30));

    controller.setStaticDisplayImage(displayPayload(QSize(80, 60)), false, renderContext());

    QVERIFY(!store->entry(previewId).has_value());
    const KiriView::ImageDisplaySourceSlot replacement = controller.snapshot().displaySource;
    QVERIFY(!replacement.providerUrl.isEmpty());
    std::optional<KiriView::DisplayImageStoreEntry> storedReplacement
        = store->entry(entryId(replacement));
    QVERIFY(storedReplacement.has_value());
    QCOMPARE(storedReplacement->quality, KiriView::DisplayImageQuality::Exact);
    QCOMPARE(storedReplacement->previewOrigin, KiriView::DisplayImagePreviewOrigin::None);
}

void TestImagePageSurfaceController::rawShadowThumbnailPreviewEntryIsReleasedOnDecodedReplacement()
{
    auto store = std::make_shared<KiriView::DisplayImageStore>(testByteBudget);
    KiriView::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);
    KiriView::StaticDisplayImagePayload preview = displayPayload(QSize(32, 32));
    preview.image = KiriView::TestSupport::testImage(QSize(16, 16));
    preview.quality = KiriView::DisplayImageQuality::ThumbnailPreview;
    preview.previewOrigin = KiriView::DisplayImagePreviewOrigin::RawEmbeddedThumbnail;

    const QString previewId = controller.publishShadowDisplayImage(std::move(preview));

    QVERIFY(!previewId.isEmpty());
    QCOMPARE(
        controller.snapshot().displaySource.status, KiriView::ImageDisplaySourceStatus::Missing);
    std::optional<KiriView::DisplayImageStoreEntry> storedPreview = store->entry(previewId);
    QVERIFY(storedPreview.has_value());
    QCOMPARE(storedPreview->quality, KiriView::DisplayImageQuality::ThumbnailPreview);
    QCOMPARE(
        storedPreview->previewOrigin, KiriView::DisplayImagePreviewOrigin::RawEmbeddedThumbnail);
    QCOMPARE(storedPreview->originalSize, QSize(32, 32));
    QCOMPARE(storedPreview->rasterSize, QSize(16, 16));

    controller.setStaticDisplayImage(displayPayload(QSize(32, 32)), false, renderContext());

    QVERIFY(!store->entry(previewId).has_value());
    const KiriView::ImageDisplaySourceSlot replacement = controller.snapshot().displaySource;
    QVERIFY(!replacement.providerUrl.isEmpty());
    std::optional<KiriView::DisplayImageStoreEntry> storedReplacement
        = store->entry(entryId(replacement));
    QVERIFY(storedReplacement.has_value());
    QCOMPARE(storedReplacement->quality, KiriView::DisplayImageQuality::Exact);
    QCOMPARE(storedReplacement->previewOrigin, KiriView::DisplayImagePreviewOrigin::None);
}

void TestImagePageSurfaceController::qtRasterFirstDisplayRefinesToProviderBucket()
{
    auto store = std::make_shared<KiriView::DisplayImageStore>(testByteBudget);
    KiriView::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);

    controller.setStaticDisplayImage(
        qtRasterPayload(QSize(16, 12), QSize(4, 3), QStringLiteral("source-a")), false,
        renderContext());
    const KiriView::ImageDisplaySourceSlot first = controller.snapshot().displaySource;
    QVERIFY(!first.providerUrl.isEmpty());
    QCOMPARE(first.rasterSize, QSize(4, 3));

    controller.scheduleVisibleTileDecode(visibleProjection(QSizeF(8.0, 6.0)));

    QTRY_VERIFY(controller.snapshot().displaySource.providerUrl != first.providerUrl);
    const KiriView::ImageDisplaySourceSlot refined = controller.snapshot().displaySource;
    QCOMPARE(refined.sourceIdentity, QStringLiteral("source-a"));
    QCOMPARE(refined.rasterSize, QSize(8, 6));
    QCOMPARE(refined.revision, first.revision + 1);
    QCOMPARE(refined.quality, KiriView::DisplayImageQuality::Exact);
    QVERIFY(!store->entry(entryId(first)).has_value());
    QVERIFY(store->entry(entryId(refined)).has_value());
}

void TestImagePageSurfaceController::qtRasterRefinementCompletionIsRejectedAfterSourceReplacement()
{
    auto store = std::make_shared<KiriView::DisplayImageStore>(testByteBudget);
    KiriView::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);

    controller.setStaticDisplayImage(
        qtRasterPayload(QSize(16, 12), QSize(4, 3), QStringLiteral("source-a")), false,
        renderContext());
    controller.scheduleVisibleTileDecode(visibleProjection(QSizeF(8.0, 6.0)));

    controller.setStaticDisplayImage(
        qtRasterPayload(QSize(10, 10), QSize(10, 10), QStringLiteral("source-b"),
            KiriView::DisplayImageQuality::Exact),
        false, renderContext());
    const KiriView::ImageDisplaySourceSlot replacement = controller.snapshot().displaySource;

    QTest::qWait(100);

    const KiriView::ImageDisplaySourceSlot current = controller.snapshot().displaySource;
    QCOMPARE(current.providerUrl, replacement.providerUrl);
    QCOMPARE(current.sourceIdentity, QStringLiteral("source-b"));
    QCOMPARE(current.rasterSize, QSize(10, 10));
    QCOMPARE(current.quality, KiriView::DisplayImageQuality::Exact);
}

void TestImagePageSurfaceController::exactQtRasterCurrentImageDoesNotRequestRefinement()
{
    auto store = std::make_shared<KiriView::DisplayImageStore>(testByteBudget);
    KiriView::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);

    controller.setStaticDisplayImage(
        qtRasterPayload(QSize(8, 6), QSize(8, 6), QStringLiteral("source-exact"),
            KiriView::DisplayImageQuality::Exact),
        false, renderContext());
    const KiriView::ImageDisplaySourceSlot exact = controller.snapshot().displaySource;

    controller.scheduleVisibleTileDecode(visibleProjection(QSizeF(8.0, 6.0)));
    QTest::qWait(100);

    const KiriView::ImageDisplaySourceSlot current = controller.snapshot().displaySource;
    QCOMPARE(current.providerUrl, exact.providerUrl);
    QCOMPARE(current.revision, exact.revision);
    QCOMPARE(current.rasterSize, QSize(8, 6));
}

void TestImagePageSurfaceController::heifFirstDisplayRefinesToProviderBucket()
{
    auto store = std::make_shared<KiriView::DisplayImageStore>(testByteBudget);
    KiriView::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);

    std::optional<KiriView::StaticDisplayImagePayload> payload
        = heifPayload(QSize(16, 12), QSize(4, 3), QStringLiteral("heif-source-a"));
    QVERIFY2(payload.has_value(), "HEIF still fixture could not be created");
    controller.setStaticDisplayImage(std::move(*payload), false, renderContext());
    const KiriView::ImageDisplaySourceSlot first = controller.snapshot().displaySource;
    QVERIFY(!first.providerUrl.isEmpty());
    QCOMPARE(first.rasterSize, QSize(4, 3));

    controller.scheduleVisibleTileDecode(visibleProjection(QSizeF(8.0, 6.0)));

    QTRY_VERIFY(controller.snapshot().displaySource.providerUrl != first.providerUrl);
    const KiriView::ImageDisplaySourceSlot refined = controller.snapshot().displaySource;
    QCOMPARE(refined.sourceIdentity, QStringLiteral("heif-source-a"));
    QCOMPARE(refined.rasterSize, QSize(8, 6));
    QCOMPARE(refined.revision, first.revision + 1);
    QCOMPARE(refined.quality, KiriView::DisplayImageQuality::Exact);
    QVERIFY(!store->entry(entryId(first)).has_value());
    QVERIFY(store->entry(entryId(refined)).has_value());
}

void TestImagePageSurfaceController::heifRefinementCompletionIsRejectedAfterSourceReplacement()
{
    auto store = std::make_shared<KiriView::DisplayImageStore>(testByteBudget);
    KiriView::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);

    std::optional<KiriView::StaticDisplayImagePayload> payload
        = heifPayload(QSize(16, 12), QSize(4, 3), QStringLiteral("heif-source-a"));
    QVERIFY2(payload.has_value(), "HEIF still fixture could not be created");
    controller.setStaticDisplayImage(std::move(*payload), false, renderContext());
    controller.scheduleVisibleTileDecode(visibleProjection(QSizeF(8.0, 6.0)));

    controller.setStaticDisplayImage(
        qtRasterPayload(QSize(10, 10), QSize(10, 10), QStringLiteral("source-b"),
            KiriView::DisplayImageQuality::Exact),
        false, renderContext());
    const KiriView::ImageDisplaySourceSlot replacement = controller.snapshot().displaySource;

    QTest::qWait(100);

    const KiriView::ImageDisplaySourceSlot current = controller.snapshot().displaySource;
    QCOMPARE(current.providerUrl, replacement.providerUrl);
    QCOMPARE(current.sourceIdentity, QStringLiteral("source-b"));
    QCOMPARE(current.rasterSize, QSize(10, 10));
    QCOMPARE(current.quality, KiriView::DisplayImageQuality::Exact);
}

void TestImagePageSurfaceController::rawFirstDisplayRefinesToProviderBucket()
{
    auto store = std::make_shared<KiriView::DisplayImageStore>(testByteBudget);
    KiriView::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);

    std::optional<KiriView::StaticDisplayImagePayload> payload
        = rawPayload(QSize(8, 8), QStringLiteral("raw-source-a"));
    QVERIFY2(payload.has_value(), "RAW still fixture could not be decoded");
    controller.setStaticDisplayImage(std::move(*payload), false, renderContext());
    const KiriView::ImageDisplaySourceSlot first = controller.snapshot().displaySource;
    QVERIFY(!first.providerUrl.isEmpty());
    QCOMPARE(first.rasterSize, QSize(8, 8));

    controller.scheduleVisibleTileDecode(visibleProjection(QSizeF(16.0, 16.0)));

    QTRY_VERIFY(controller.snapshot().displaySource.providerUrl != first.providerUrl);
    const KiriView::ImageDisplaySourceSlot refined = controller.snapshot().displaySource;
    QCOMPARE(refined.sourceIdentity, QStringLiteral("raw-source-a"));
    QCOMPARE(refined.rasterSize, QSize(16, 16));
    QCOMPARE(refined.revision, first.revision + 1);
    QCOMPARE(refined.quality, KiriView::DisplayImageQuality::Exact);
    QVERIFY(!store->entry(entryId(first)).has_value());
    QVERIFY(store->entry(entryId(refined)).has_value());
}

void TestImagePageSurfaceController::rawRefinementCompletionIsRejectedAfterSourceReplacement()
{
    auto store = std::make_shared<KiriView::DisplayImageStore>(testByteBudget);
    KiriView::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);

    std::optional<KiriView::StaticDisplayImagePayload> payload
        = rawPayload(QSize(8, 8), QStringLiteral("raw-source-a"));
    QVERIFY2(payload.has_value(), "RAW still fixture could not be decoded");
    controller.setStaticDisplayImage(std::move(*payload), false, renderContext());
    controller.scheduleVisibleTileDecode(visibleProjection(QSizeF(16.0, 16.0)));

    controller.setStaticDisplayImage(
        qtRasterPayload(QSize(10, 10), QSize(10, 10), QStringLiteral("source-b"),
            KiriView::DisplayImageQuality::Exact),
        false, renderContext());
    const KiriView::ImageDisplaySourceSlot replacement = controller.snapshot().displaySource;

    QTest::qWait(100);

    const KiriView::ImageDisplaySourceSlot current = controller.snapshot().displaySource;
    QCOMPARE(current.providerUrl, replacement.providerUrl);
    QCOMPARE(current.sourceIdentity, QStringLiteral("source-b"));
    QCOMPARE(current.rasterSize, QSize(10, 10));
    QCOMPARE(current.quality, KiriView::DisplayImageQuality::Exact);
}

QTEST_GUILESS_MAIN(TestImagePageSurfaceController)

#include "test_imagepagesurfacecontroller.moc"
