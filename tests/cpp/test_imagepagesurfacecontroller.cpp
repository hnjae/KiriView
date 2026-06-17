// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepagesurfacecontroller.h"

#include "async/imageworkerscheduler.h"
#include "decoding/kiriimagedecoder.h"
#include "image_test_support.h"
#include "rendering/heiftilesource.h"
#include "rendering/imagerendering.h"
#include "rendering/qimagereadertilesource.h"
#include "rendering/svgtilesource.h"

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
#include <vector>

class TestImagePageSurfaceController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void providerEntriesAreReleasedOnSupersessionAndClear();
    void setImageWithoutProviderPublishesDisplayErrorSnapshot();
    void visibleProjectionPinsAndPrioritizesProviderEntry();
    void shadowThumbnailPreviewEntryIsReleasedOnDecodedReplacement();
    void rawShadowThumbnailPreviewEntryIsReleasedOnDecodedReplacement();
    void qtRasterFirstDisplayRefinesToProviderBucket();
    void refinementPolicyUsesResolvedCacheBudgetWhenStoreBudgetDiffers();
    void qtRasterRefinementCompletionIsRejectedAfterSourceReplacement();
    void exactQtRasterCurrentImageDoesNotRequestRefinement();
    void heifFirstDisplayRefinesToProviderBucket();
    void heifRefinementCompletionIsRejectedAfterSourceReplacement();
    void rawFirstDisplayRefinesToProviderBucket();
    void rawRefinementCompletionIsRejectedAfterSourceReplacement();
    void svgFirstDisplayRefinesToCoarseProviderBucket();
    void svgRefinementCompletionIsRejectedAfterSourceReplacement();
    void stillImageLoadedOutcomeMarksAcceptedDisplaySource();
    void stillImageErrorOutcomeMarksDisplaySourceError();
    void stillImageMissingOutcomeMarksDisplaySourceMissing();
    void animationFrameProviderContractRetainsPreviousFrameUntilLoadOutcome();
    void staleAnimationFrameLoadOutcomesAreRejected();
    void animationFrameRetentionIsBoundedToPreviousFrame();
};

namespace {
constexpr qsizetype testByteBudget = 1024 * 1024;

struct ManualImageWorkerSchedule {
    kiriview::ImageWorkerOperation work;
    kiriview::ImageWorkerCompletion completion;
};

class ManualImageWorkerScheduler
{
public:
    kiriview::ImageWorkerScheduler scheduler()
    {
        return kiriview::ImageWorkerScheduler([this](QObject *, kiriview::ImageWorkerOperation work,
                                                  kiriview::ImageWorkerCompletion completion) {
            m_schedules.push_back(
                ManualImageWorkerSchedule { std::move(work), std::move(completion) });
        });
    }

    std::size_t scheduleCount() const { return m_schedules.size(); }

    void runWork(std::size_t index)
    {
        if (m_schedules.at(index).work) {
            m_schedules.at(index).work();
        }
    }

    void finish(std::size_t index)
    {
        if (m_schedules.at(index).completion) {
            m_schedules.at(index).completion();
        }
    }

private:
    std::vector<ManualImageWorkerSchedule> m_schedules;
};

kiriview::ImageCacheBudgets cacheBudgets(qsizetype displayImageBudget = testByteBudget)
{
    return kiriview::ImageCacheBudgets {
        testByteBudget,
        displayImageBudget,
        testByteBudget,
    };
}

kiriview::ImageDocumentRenderContext renderContext()
{
    return kiriview::ImageDocumentRenderContext {
        1.0,
        kiriview::fallbackTextureSizeMax,
    };
}

kiriview::ImagePresentationRenderProjection visibleProjection(const QSizeF &displaySize)
{
    kiriview::ImagePresentationRenderProjection projection;
    projection.visible = true;
    projection.displaySize = displaySize;
    projection.visibleItemRect = QRectF(0.0, 0.0, displaySize.width(), displaySize.height());
    projection.maximumTextureSize = kiriview::fallbackTextureSizeMax;
    return projection;
}

kiriview::ImagePresentationRenderProjection visibleProjection()
{
    return visibleProjection(QSizeF(8.0, 4.0));
}

kiriview::StaticDisplayImagePayload displayPayload(const QSize &size)
{
    return kiriview::TestSupport::staticDisplayTestImagePayload(
        kiriview::TestSupport::testImage(size));
}

QString entryId(const kiriview::ImageDisplaySourceSlot &slot)
{
    return slot.providerUrl.path().mid(1);
}

bool hasChange(
    const std::vector<kiriview::ImageDocumentChange> &changes, kiriview::ImageDocumentChange change)
{
    return std::find(changes.cbegin(), changes.cend(), change) != changes.cend();
}

QByteArray encodedPng(const QImage &image)
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return data;
}

QByteArray svgData(const QSize &size)
{
    return QByteArray("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"")
        + QByteArray::number(size.width()) + "\" height=\"" + QByteArray::number(size.height())
        + "\"><rect width=\"" + QByteArray::number(size.width()) + "\" height=\""
        + QByteArray::number(size.height()) + "\" fill=\"red\"/></svg>";
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
            context.get(), heif_compression_JPEG, &rawEncoder))) {
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

kiriview::StaticDisplayImagePayload qtRasterPayload(const QSize &originalSize,
    const QSize &rasterSize, const QString &sourceIdentity,
    kiriview::DisplayImageQuality quality = kiriview::DisplayImageQuality::FirstDisplay)
{
    QString errorString;
    std::shared_ptr<kiriview::QImageReaderTileSource> source
        = kiriview::QImageReaderTileSource::open(
            encodedPng(kiriview::TestSupport::testImage(originalSize)), QByteArrayLiteral("png"),
            &errorString);
    Q_ASSERT(source != nullptr);

    return kiriview::StaticDisplayImagePayload {
        sourceIdentity,
        source->imageReaderTransform(),
        originalSize,
        kiriview::TestSupport::testImage(rasterSize),
        quality,
        kiriview::imagePixelsPerSourcePixel(originalSize, rasterSize),
        {},
        std::move(source),
    };
}

kiriview::StaticDisplayImagePayload svgPayload(const QSize &originalSize, const QSize &rasterSize,
    const QString &sourceIdentity,
    kiriview::DisplayImageQuality quality = kiriview::DisplayImageQuality::FirstDisplay)
{
    QString errorString;
    std::shared_ptr<kiriview::SvgTileSource> source
        = kiriview::SvgTileSource::open(svgData(originalSize), &errorString);
    Q_ASSERT(source != nullptr);

    return kiriview::StaticDisplayImagePayload {
        sourceIdentity,
        {},
        originalSize,
        kiriview::TestSupport::testImage(rasterSize),
        quality,
        kiriview::imagePixelsPerSourcePixel(originalSize, rasterSize),
        {},
        std::move(source),
    };
}

std::optional<kiriview::StaticDisplayImagePayload> heifPayload(const QSize &originalSize,
    const QSize &rasterSize, const QString &sourceIdentity,
    kiriview::DisplayImageQuality quality = kiriview::DisplayImageQuality::FirstDisplay)
{
    QString errorString;
    std::shared_ptr<kiriview::ImageTileSource> source
        = kiriview::openHeifTileSource(encodedHeifStill(originalSize), &errorString);
    if (source == nullptr) {
        return std::nullopt;
    }

    return kiriview::StaticDisplayImagePayload {
        sourceIdentity,
        {},
        originalSize,
        kiriview::TestSupport::testImage(rasterSize),
        quality,
        kiriview::imagePixelsPerSourcePixel(originalSize, rasterSize),
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

std::optional<kiriview::StaticDisplayImagePayload> rawPayload(const QSize &rasterSize,
    const QString &sourceIdentity,
    kiriview::DisplayImageQuality quality = kiriview::DisplayImageQuality::FirstDisplay)
{
    const QByteArray data = rawFixtureData();
    if (data.isEmpty()) {
        return std::nullopt;
    }

    const kiriview::ImageDecodeRequest request = kiriview::ImageDecodeRequest::fromUrl(71,
        QUrl::fromLocalFile(
            QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR "/../fixtures/raw-cfa-smoke.dng")));
    const kiriview::DecodedImageResult result = kiriview::decodeImageData(data, request);
    const auto *decoded = kiriview::decodedImageResultImageAs<kiriview::StaticDecodedImage>(result);
    if (decoded == nullptr || decoded->displayImage.refinementSource == nullptr) {
        return std::nullopt;
    }

    kiriview::StaticDisplayImagePayload payload = decoded->displayImage;
    payload.sourceIdentity = sourceIdentity;
    payload.image = kiriview::displayReadyImage(
        payload.image.scaled(rasterSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    payload.quality = quality;
    payload.displayPixelsPerSourcePixel
        = kiriview::imagePixelsPerSourcePixel(payload.originalSize, payload.image.size());
    payload.previewOrigin = kiriview::DisplayImagePreviewOrigin::None;
    return payload;
}
}

void TestImagePageSurfaceController::providerEntriesAreReleasedOnSupersessionAndClear()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);

    controller.setStaticDisplayImage(displayPayload(QSize(8, 4)), false, renderContext());
    const kiriview::ImageDisplaySourceSlot first = controller.snapshot().displaySource();
    QVERIFY(!first.providerUrl.isEmpty());
    QVERIFY(store->entry(entryId(first)).has_value());

    controller.setStaticDisplayImage(displayPayload(QSize(6, 4)), false, renderContext());
    const kiriview::ImageDisplaySourceSlot second = controller.snapshot().displaySource();
    QVERIFY(!second.providerUrl.isEmpty());
    QVERIFY(first.providerUrl != second.providerUrl);
    QCOMPARE(second.revision, quint64(2));
    QVERIFY(!store->entry(entryId(first)).has_value());
    QVERIFY(store->entry(entryId(second)).has_value());

    controller.clearImage();
    QVERIFY(!store->entry(entryId(second)).has_value());
    QCOMPARE(
        controller.snapshot().displaySource().status, kiriview::ImageDisplaySourceStatus::Missing);
}

void TestImagePageSurfaceController::setImageWithoutProviderPublishesDisplayErrorSnapshot()
{
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets());

    controller.setImage(kiriview::TestSupport::testImage(QSize(8, 4)), false);

    const kiriview::ImagePresentationPageSlotSnapshot snapshot = controller.snapshot();
    QVERIFY(snapshot.hasImage());
    QCOMPARE(snapshot.imageSize, QSize(8, 4));
    QVERIFY(snapshot.displaySource().providerUrl.isEmpty());
    QCOMPARE(snapshot.displaySource().originalSize, QSize(8, 4));
    QCOMPARE(snapshot.displaySource().rasterSize, QSize(8, 4));
    QCOMPARE(snapshot.displaySource().status, kiriview::ImageDisplaySourceStatus::Error);
}

void TestImagePageSurfaceController::visibleProjectionPinsAndPrioritizesProviderEntry()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(128);
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets(128), store);

    controller.setStaticDisplayImage(displayPayload(QSize(8, 4)), false, renderContext());
    const kiriview::ImageDisplaySourceSlot slot = controller.snapshot().displaySource();
    const QString id = entryId(slot);
    QVERIFY(store->entry(id).has_value());
    QVERIFY(slot.loadAcknowledgmentRequired);

    controller.updateDisplayProjection(visibleProjection());
    std::optional<kiriview::DisplayImageStoreEntry> stored = store->entry(id);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->priority, kiriview::DisplayImageRetentionPriority::Visible);

    const QString background = store->insert(kiriview::DisplayImageEntry {
        kiriview::TestSupport::testImage(QSize(8, 4)),
        QSize(8, 4),
        QSize(8, 4),
        QStringLiteral("background"),
        kiriview::DisplayedPageRole::Primary,
        kiriview::DisplayImageQuality::Exact,
        kiriview::DisplayImageRetentionPriority::Background,
        1,
        QStringLiteral("background"),
    });
    Q_UNUSED(background);

    QVERIFY(store->entry(id).has_value());

    kiriview::ImagePresentationRenderProjection hidden;
    controller.updateDisplayProjection(hidden);
    stored = store->entry(id);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->priority, kiriview::DisplayImageRetentionPriority::Nearby);
}

void TestImagePageSurfaceController::shadowThumbnailPreviewEntryIsReleasedOnDecodedReplacement()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);
    kiriview::StaticDisplayImagePayload preview = displayPayload(QSize(80, 60));
    preview.image = kiriview::TestSupport::testImage(QSize(40, 30));
    preview.quality = kiriview::DisplayImageQuality::ThumbnailPreview;
    preview.previewOrigin = kiriview::DisplayImagePreviewOrigin::XdgThumbnail;

    const QString previewId = controller.publishShadowDisplayImage(std::move(preview));

    QVERIFY(!previewId.isEmpty());
    QCOMPARE(
        controller.snapshot().displaySource().status, kiriview::ImageDisplaySourceStatus::Missing);
    std::optional<kiriview::DisplayImageStoreEntry> storedPreview = store->entry(previewId);
    QVERIFY(storedPreview.has_value());
    QCOMPARE(storedPreview->quality, kiriview::DisplayImageQuality::ThumbnailPreview);
    QCOMPARE(storedPreview->previewOrigin, kiriview::DisplayImagePreviewOrigin::XdgThumbnail);
    QCOMPARE(storedPreview->originalSize, QSize(80, 60));
    QCOMPARE(storedPreview->rasterSize, QSize(40, 30));

    controller.setStaticDisplayImage(displayPayload(QSize(80, 60)), false, renderContext());

    QVERIFY(!store->entry(previewId).has_value());
    const kiriview::ImageDisplaySourceSlot replacement = controller.snapshot().displaySource();
    QVERIFY(!replacement.providerUrl.isEmpty());
    std::optional<kiriview::DisplayImageStoreEntry> storedReplacement
        = store->entry(entryId(replacement));
    QVERIFY(storedReplacement.has_value());
    QCOMPARE(storedReplacement->quality, kiriview::DisplayImageQuality::Exact);
    QCOMPARE(storedReplacement->previewOrigin, kiriview::DisplayImagePreviewOrigin::None);
}

void TestImagePageSurfaceController::rawShadowThumbnailPreviewEntryIsReleasedOnDecodedReplacement()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);
    kiriview::StaticDisplayImagePayload preview = displayPayload(QSize(32, 32));
    preview.image = kiriview::TestSupport::testImage(QSize(16, 16));
    preview.quality = kiriview::DisplayImageQuality::ThumbnailPreview;
    preview.previewOrigin = kiriview::DisplayImagePreviewOrigin::RawEmbeddedThumbnail;

    const QString previewId = controller.publishShadowDisplayImage(std::move(preview));

    QVERIFY(!previewId.isEmpty());
    QCOMPARE(
        controller.snapshot().displaySource().status, kiriview::ImageDisplaySourceStatus::Missing);
    std::optional<kiriview::DisplayImageStoreEntry> storedPreview = store->entry(previewId);
    QVERIFY(storedPreview.has_value());
    QCOMPARE(storedPreview->quality, kiriview::DisplayImageQuality::ThumbnailPreview);
    QCOMPARE(
        storedPreview->previewOrigin, kiriview::DisplayImagePreviewOrigin::RawEmbeddedThumbnail);
    QCOMPARE(storedPreview->originalSize, QSize(32, 32));
    QCOMPARE(storedPreview->rasterSize, QSize(16, 16));

    controller.setStaticDisplayImage(displayPayload(QSize(32, 32)), false, renderContext());

    QVERIFY(!store->entry(previewId).has_value());
    const kiriview::ImageDisplaySourceSlot replacement = controller.snapshot().displaySource();
    QVERIFY(!replacement.providerUrl.isEmpty());
    std::optional<kiriview::DisplayImageStoreEntry> storedReplacement
        = store->entry(entryId(replacement));
    QVERIFY(storedReplacement.has_value());
    QCOMPARE(storedReplacement->quality, kiriview::DisplayImageQuality::Exact);
    QCOMPARE(storedReplacement->previewOrigin, kiriview::DisplayImagePreviewOrigin::None);
}

void TestImagePageSurfaceController::qtRasterFirstDisplayRefinesToProviderBucket()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    ManualImageWorkerScheduler workerScheduler;
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store,
        kiriview::DisplayedPageRole::Primary, workerScheduler.scheduler());

    controller.setStaticDisplayImage(
        qtRasterPayload(QSize(16, 12), QSize(4, 3), QStringLiteral("source-a")), false,
        renderContext());
    const kiriview::ImageDisplaySourceSlot first = controller.snapshot().displaySource();
    QVERIFY(!first.providerUrl.isEmpty());
    QCOMPARE(first.rasterSize, QSize(4, 3));

    controller.updateDisplayProjection(visibleProjection(QSizeF(8.0, 6.0)));

    QCOMPARE(workerScheduler.scheduleCount(), std::size_t(1));
    QCOMPARE(controller.snapshot().displaySource().providerUrl, first.providerUrl);

    workerScheduler.runWork(0);

    QCOMPARE(controller.snapshot().displaySource().providerUrl, first.providerUrl);

    workerScheduler.finish(0);

    QVERIFY(controller.snapshot().displaySource().providerUrl != first.providerUrl);
    const kiriview::ImageDisplaySourceSlot refined = controller.snapshot().displaySource();
    QCOMPARE(refined.sourceIdentity, QStringLiteral("source-a"));
    QCOMPARE(refined.rasterSize, QSize(8, 6));
    QCOMPARE(refined.revision, first.revision + 1);
    QCOMPARE(refined.quality, kiriview::DisplayImageQuality::Exact);
    QVERIFY(!store->entry(entryId(first)).has_value());
    QVERIFY(store->entry(entryId(refined)).has_value());
}

void TestImagePageSurfaceController::refinementPolicyUsesResolvedCacheBudgetWhenStoreBudgetDiffers()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets(1), store);

    controller.setStaticDisplayImage(
        qtRasterPayload(QSize(16, 12), QSize(4, 3), QStringLiteral("source-a")), false,
        renderContext());
    const kiriview::ImageDisplaySourceSlot first = controller.snapshot().displaySource();
    QVERIFY(!first.providerUrl.isEmpty());
    QCOMPARE(first.rasterSize, QSize(4, 3));

    controller.updateDisplayProjection(visibleProjection(QSizeF(8.0, 6.0)));
    QTest::qWait(100);

    const kiriview::ImageDisplaySourceSlot current = controller.snapshot().displaySource();
    QCOMPARE(current.providerUrl, first.providerUrl);
    QCOMPARE(current.rasterSize, QSize(4, 3));
    QVERIFY(store->entry(entryId(first)).has_value());
}

void TestImagePageSurfaceController::qtRasterRefinementCompletionIsRejectedAfterSourceReplacement()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);

    controller.setStaticDisplayImage(
        qtRasterPayload(QSize(16, 12), QSize(4, 3), QStringLiteral("source-a")), false,
        renderContext());
    controller.updateDisplayProjection(visibleProjection(QSizeF(8.0, 6.0)));

    controller.setStaticDisplayImage(
        qtRasterPayload(QSize(10, 10), QSize(10, 10), QStringLiteral("source-b"),
            kiriview::DisplayImageQuality::Exact),
        false, renderContext());
    const kiriview::ImageDisplaySourceSlot replacement = controller.snapshot().displaySource();

    QTest::qWait(100);

    const kiriview::ImageDisplaySourceSlot current = controller.snapshot().displaySource();
    QCOMPARE(current.providerUrl, replacement.providerUrl);
    QCOMPARE(current.sourceIdentity, QStringLiteral("source-b"));
    QCOMPARE(current.rasterSize, QSize(10, 10));
    QCOMPARE(current.quality, kiriview::DisplayImageQuality::Exact);
}

void TestImagePageSurfaceController::exactQtRasterCurrentImageDoesNotRequestRefinement()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);

    controller.setStaticDisplayImage(
        qtRasterPayload(QSize(8, 6), QSize(8, 6), QStringLiteral("source-exact"),
            kiriview::DisplayImageQuality::Exact),
        false, renderContext());
    const kiriview::ImageDisplaySourceSlot exact = controller.snapshot().displaySource();

    controller.updateDisplayProjection(visibleProjection(QSizeF(8.0, 6.0)));
    QTest::qWait(100);

    const kiriview::ImageDisplaySourceSlot current = controller.snapshot().displaySource();
    QCOMPARE(current.providerUrl, exact.providerUrl);
    QCOMPARE(current.revision, exact.revision);
    QCOMPARE(current.rasterSize, QSize(8, 6));
}

void TestImagePageSurfaceController::heifFirstDisplayRefinesToProviderBucket()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);

    std::optional<kiriview::StaticDisplayImagePayload> payload
        = heifPayload(QSize(16, 12), QSize(4, 3), QStringLiteral("heif-source-a"));
    QVERIFY2(payload.has_value(), "HEIF still fixture could not be created");
    controller.setStaticDisplayImage(std::move(*payload), false, renderContext());
    const kiriview::ImageDisplaySourceSlot first = controller.snapshot().displaySource();
    QVERIFY(!first.providerUrl.isEmpty());
    QCOMPARE(first.rasterSize, QSize(4, 3));

    controller.updateDisplayProjection(visibleProjection(QSizeF(8.0, 6.0)));

    QTRY_VERIFY(controller.snapshot().displaySource().providerUrl != first.providerUrl);
    const kiriview::ImageDisplaySourceSlot refined = controller.snapshot().displaySource();
    QCOMPARE(refined.sourceIdentity, QStringLiteral("heif-source-a"));
    QCOMPARE(refined.rasterSize, QSize(8, 6));
    QCOMPARE(refined.revision, first.revision + 1);
    QCOMPARE(refined.quality, kiriview::DisplayImageQuality::Exact);
    QVERIFY(!store->entry(entryId(first)).has_value());
    QVERIFY(store->entry(entryId(refined)).has_value());
}

void TestImagePageSurfaceController::heifRefinementCompletionIsRejectedAfterSourceReplacement()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);

    std::optional<kiriview::StaticDisplayImagePayload> payload
        = heifPayload(QSize(16, 12), QSize(4, 3), QStringLiteral("heif-source-a"));
    QVERIFY2(payload.has_value(), "HEIF still fixture could not be created");
    controller.setStaticDisplayImage(std::move(*payload), false, renderContext());
    controller.updateDisplayProjection(visibleProjection(QSizeF(8.0, 6.0)));

    controller.setStaticDisplayImage(
        qtRasterPayload(QSize(10, 10), QSize(10, 10), QStringLiteral("source-b"),
            kiriview::DisplayImageQuality::Exact),
        false, renderContext());
    const kiriview::ImageDisplaySourceSlot replacement = controller.snapshot().displaySource();

    QTest::qWait(100);

    const kiriview::ImageDisplaySourceSlot current = controller.snapshot().displaySource();
    QCOMPARE(current.providerUrl, replacement.providerUrl);
    QCOMPARE(current.sourceIdentity, QStringLiteral("source-b"));
    QCOMPARE(current.rasterSize, QSize(10, 10));
    QCOMPARE(current.quality, kiriview::DisplayImageQuality::Exact);
}

void TestImagePageSurfaceController::rawFirstDisplayRefinesToProviderBucket()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);

    std::optional<kiriview::StaticDisplayImagePayload> payload
        = rawPayload(QSize(8, 8), QStringLiteral("raw-source-a"));
    QVERIFY2(payload.has_value(), "RAW still fixture could not be decoded");
    controller.setStaticDisplayImage(std::move(*payload), false, renderContext());
    const kiriview::ImageDisplaySourceSlot first = controller.snapshot().displaySource();
    QVERIFY(!first.providerUrl.isEmpty());
    QCOMPARE(first.rasterSize, QSize(8, 8));

    controller.updateDisplayProjection(visibleProjection(QSizeF(16.0, 16.0)));

    QTRY_VERIFY(controller.snapshot().displaySource().providerUrl != first.providerUrl);
    const kiriview::ImageDisplaySourceSlot refined = controller.snapshot().displaySource();
    QCOMPARE(refined.sourceIdentity, QStringLiteral("raw-source-a"));
    QCOMPARE(refined.rasterSize, QSize(16, 16));
    QCOMPARE(refined.revision, first.revision + 1);
    QCOMPARE(refined.quality, kiriview::DisplayImageQuality::Exact);
    QVERIFY(!store->entry(entryId(first)).has_value());
    QVERIFY(store->entry(entryId(refined)).has_value());
}

void TestImagePageSurfaceController::rawRefinementCompletionIsRejectedAfterSourceReplacement()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);

    std::optional<kiriview::StaticDisplayImagePayload> payload
        = rawPayload(QSize(8, 8), QStringLiteral("raw-source-a"));
    QVERIFY2(payload.has_value(), "RAW still fixture could not be decoded");
    controller.setStaticDisplayImage(std::move(*payload), false, renderContext());
    controller.updateDisplayProjection(visibleProjection(QSizeF(16.0, 16.0)));

    controller.setStaticDisplayImage(
        qtRasterPayload(QSize(10, 10), QSize(10, 10), QStringLiteral("source-b"),
            kiriview::DisplayImageQuality::Exact),
        false, renderContext());
    const kiriview::ImageDisplaySourceSlot replacement = controller.snapshot().displaySource();

    QTest::qWait(100);

    const kiriview::ImageDisplaySourceSlot current = controller.snapshot().displaySource();
    QCOMPARE(current.providerUrl, replacement.providerUrl);
    QCOMPARE(current.sourceIdentity, QStringLiteral("source-b"));
    QCOMPARE(current.rasterSize, QSize(10, 10));
    QCOMPARE(current.quality, kiriview::DisplayImageQuality::Exact);
}

void TestImagePageSurfaceController::svgFirstDisplayRefinesToCoarseProviderBucket()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);

    controller.setStaticDisplayImage(
        svgPayload(QSize(80, 40), QSize(80, 40), QStringLiteral("svg-source-a")), false,
        renderContext());
    const kiriview::ImageDisplaySourceSlot first = controller.snapshot().displaySource();
    QVERIFY(!first.providerUrl.isEmpty());
    QCOMPARE(first.rasterSize, QSize(80, 40));

    controller.updateDisplayProjection(visibleProjection(QSizeF(100.0, 50.0)));

    QTRY_VERIFY(controller.snapshot().displaySource().providerUrl != first.providerUrl);
    const kiriview::ImageDisplaySourceSlot refined = controller.snapshot().displaySource();
    QCOMPARE(refined.sourceIdentity, QStringLiteral("svg-source-a"));
    QCOMPARE(refined.rasterSize, QSize(120, 60));
    QCOMPARE(refined.revision, first.revision + 1);
    QCOMPARE(refined.quality, kiriview::DisplayImageQuality::Exact);
    QVERIFY(!store->entry(entryId(first)).has_value());
    QVERIFY(store->entry(entryId(refined)).has_value());

    controller.updateDisplayProjection(visibleProjection(QSizeF(110.0, 55.0)));
    QTest::qWait(100);
    QCOMPARE(controller.snapshot().displaySource().providerUrl, refined.providerUrl);

    controller.updateDisplayProjection(visibleProjection(QSizeF(150.0, 75.0)));
    QTRY_VERIFY(controller.snapshot().displaySource().providerUrl != refined.providerUrl);
    const kiriview::ImageDisplaySourceSlot sharper = controller.snapshot().displaySource();
    QCOMPARE(sharper.rasterSize, QSize(180, 90));
    QCOMPARE(sharper.revision, refined.revision + 1);
}

void TestImagePageSurfaceController::svgRefinementCompletionIsRejectedAfterSourceReplacement()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);

    controller.setStaticDisplayImage(
        svgPayload(QSize(80, 40), QSize(80, 40), QStringLiteral("svg-source-a")), false,
        renderContext());
    controller.updateDisplayProjection(visibleProjection(QSizeF(100.0, 50.0)));

    controller.setStaticDisplayImage(
        qtRasterPayload(QSize(10, 10), QSize(10, 10), QStringLiteral("source-b"),
            kiriview::DisplayImageQuality::Exact),
        false, renderContext());
    const kiriview::ImageDisplaySourceSlot replacement = controller.snapshot().displaySource();

    QTest::qWait(100);

    const kiriview::ImageDisplaySourceSlot current = controller.snapshot().displaySource();
    QCOMPARE(current.providerUrl, replacement.providerUrl);
    QCOMPARE(current.sourceIdentity, QStringLiteral("source-b"));
    QCOMPARE(current.rasterSize, QSize(10, 10));
    QCOMPARE(current.quality, kiriview::DisplayImageQuality::Exact);
}

void TestImagePageSurfaceController::stillImageLoadedOutcomeMarksAcceptedDisplaySource()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImagePageSurfaceController controller(this,
        kiriview::ImagePageSurfaceController::Callbacks {
            [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); },
            {},
        },
        cacheBudgets(), store);

    controller.setStaticDisplayImage(displayPayload(QSize(8, 4)), false, renderContext());
    const kiriview::ImageDisplaySourceSlot ready = controller.snapshot().displaySource();
    QVERIFY(ready.loadAcknowledgmentRequired);
    changes.clear();

    controller.acknowledgeStillImageDisplayLoad(ready.providerUrl, ready.revision,
        ready.sourceIdentity, kiriview::ImageDisplayLoadOutcome::Loaded);

    const kiriview::ImageDisplaySourceSlot loaded = controller.snapshot().displaySource();
    QCOMPARE(loaded.providerUrl, ready.providerUrl);
    QCOMPARE(loaded.revision, ready.revision);
    QCOMPARE(loaded.status, kiriview::ImageDisplaySourceStatus::Ready);
    QVERIFY(!loaded.loadAcknowledgmentRequired);
    QVERIFY(store->entry(entryId(loaded)).has_value());
    QVERIFY(hasChange(changes, kiriview::ImageDocumentChange::DisplaySource));
}

void TestImagePageSurfaceController::stillImageErrorOutcomeMarksDisplaySourceError()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImagePageSurfaceController controller(this,
        kiriview::ImagePageSurfaceController::Callbacks {
            [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); },
            {},
        },
        cacheBudgets(), store);

    controller.setStaticDisplayImage(displayPayload(QSize(8, 4)), false, renderContext());
    const kiriview::ImageDisplaySourceSlot ready = controller.snapshot().displaySource();
    changes.clear();

    controller.acknowledgeStillImageDisplayLoad(ready.providerUrl, ready.revision,
        ready.sourceIdentity, kiriview::ImageDisplayLoadOutcome::Error);

    const kiriview::ImageDisplaySourceSlot failed = controller.snapshot().displaySource();
    QCOMPARE(failed.providerUrl, ready.providerUrl);
    QCOMPARE(failed.revision, ready.revision);
    QCOMPARE(failed.status, kiriview::ImageDisplaySourceStatus::Error);
    QVERIFY(!failed.loadAcknowledgmentRequired);
    QVERIFY(hasChange(changes, kiriview::ImageDocumentChange::DisplaySource));
}

void TestImagePageSurfaceController::stillImageMissingOutcomeMarksDisplaySourceMissing()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImagePageSurfaceController controller(this,
        kiriview::ImagePageSurfaceController::Callbacks {
            [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); },
            {},
        },
        cacheBudgets(), store);

    controller.setStaticDisplayImage(displayPayload(QSize(8, 4)), false, renderContext());
    const kiriview::ImageDisplaySourceSlot ready = controller.snapshot().displaySource();
    changes.clear();

    controller.acknowledgeStillImageDisplayLoad(ready.providerUrl, ready.revision,
        ready.sourceIdentity, kiriview::ImageDisplayLoadOutcome::Missing);

    const kiriview::ImageDisplaySourceSlot missing = controller.snapshot().displaySource();
    QCOMPARE(missing.providerUrl, ready.providerUrl);
    QCOMPARE(missing.revision, ready.revision);
    QCOMPARE(missing.status, kiriview::ImageDisplaySourceStatus::Missing);
    QVERIFY(!missing.loadAcknowledgmentRequired);
    QVERIFY(hasChange(changes, kiriview::ImageDocumentChange::DisplaySource));
}

void TestImagePageSurfaceController::
    animationFrameProviderContractRetainsPreviousFrameUntilLoadOutcome()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);
    const QString sourceIdentity = QStringLiteral("file:///tmp/animated.apng");

    controller.setAnimationFrame(kiriview::TestSupport::testImage(QSize(4, 4)), sourceIdentity);
    const kiriview::ImageDisplaySourceSlot first = controller.snapshot().displaySource();
    const QString firstId = entryId(first);
    QVERIFY(first.loadAcknowledgmentRequired);
    QVERIFY(store->entry(firstId).has_value());

    controller.setAnimationFrame(kiriview::TestSupport::testImage(QSize(5, 4)), sourceIdentity);
    const kiriview::ImageDisplaySourceSlot second = controller.snapshot().displaySource();
    const QString secondId = entryId(second);
    QVERIFY(second.loadAcknowledgmentRequired);
    QVERIFY(first.providerUrl != second.providerUrl);
    QVERIFY(store->entry(firstId).has_value());
    QVERIFY(store->entry(secondId).has_value());

    controller.acknowledgeAnimationFrameDisplayLoad(second.providerUrl, second.revision,
        sourceIdentity, kiriview::ImageDisplayLoadOutcome::Loaded);

    QVERIFY(!store->entry(firstId).has_value());
    QVERIFY(store->entry(secondId).has_value());
    QCOMPARE(controller.snapshot().displaySource().providerUrl, second.providerUrl);
}

void TestImagePageSurfaceController::staleAnimationFrameLoadOutcomesAreRejected()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);
    const QString sourceIdentity = QStringLiteral("file:///tmp/animated.apng");

    controller.setAnimationFrame(kiriview::TestSupport::testImage(QSize(4, 4)), sourceIdentity);
    const kiriview::ImageDisplaySourceSlot first = controller.snapshot().displaySource();
    const QString firstId = entryId(first);

    controller.setAnimationFrame(kiriview::TestSupport::testImage(QSize(5, 4)), sourceIdentity);
    const kiriview::ImageDisplaySourceSlot second = controller.snapshot().displaySource();
    QVERIFY(store->entry(firstId).has_value());

    controller.acknowledgeAnimationFrameDisplayLoad(first.providerUrl, first.revision,
        sourceIdentity, kiriview::ImageDisplayLoadOutcome::Loaded);
    QVERIFY(store->entry(firstId).has_value());

    controller.acknowledgeAnimationFrameDisplayLoad(second.providerUrl, second.revision,
        QStringLiteral("file:///tmp/replacement.apng"), kiriview::ImageDisplayLoadOutcome::Loaded);
    QVERIFY(store->entry(firstId).has_value());

    controller.acknowledgeAnimationFrameDisplayLoad(second.providerUrl, second.revision,
        sourceIdentity, kiriview::ImageDisplayLoadOutcome::Loaded);
    QVERIFY(!store->entry(firstId).has_value());
}

void TestImagePageSurfaceController::animationFrameRetentionIsBoundedToPreviousFrame()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(testByteBudget);
    kiriview::ImagePageSurfaceController controller(this, {}, cacheBudgets(), store);
    const QString sourceIdentity = QStringLiteral("file:///tmp/animated.apng");

    controller.setAnimationFrame(kiriview::TestSupport::testImage(QSize(4, 4)), sourceIdentity);
    const kiriview::ImageDisplaySourceSlot first = controller.snapshot().displaySource();
    const QString firstId = entryId(first);

    controller.setAnimationFrame(kiriview::TestSupport::testImage(QSize(5, 4)), sourceIdentity);
    const kiriview::ImageDisplaySourceSlot second = controller.snapshot().displaySource();
    const QString secondId = entryId(second);
    QVERIFY(store->entry(firstId).has_value());

    controller.setAnimationFrame(kiriview::TestSupport::testImage(QSize(6, 4)), sourceIdentity);
    const kiriview::ImageDisplaySourceSlot third = controller.snapshot().displaySource();
    const QString thirdId = entryId(third);

    QVERIFY(!store->entry(firstId).has_value());
    QVERIFY(store->entry(secondId).has_value());
    QVERIFY(store->entry(thirdId).has_value());
    QCOMPARE(store->size(), qsizetype(2));
}

QTEST_GUILESS_MAIN(TestImagePageSurfaceController)

#include "test_imagepagesurfacecontroller.moc"
