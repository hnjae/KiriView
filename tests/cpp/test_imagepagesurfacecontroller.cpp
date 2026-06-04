// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepagesurfacecontroller.h"

#include "image_test_support.h"

#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QTest>
#include <memory>
#include <optional>

class TestImagePageSurfaceController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void providerEntriesAreReleasedOnSupersessionAndClear();
    void visibleProjectionPinsAndPrioritizesProviderEntry();
    void shadowThumbnailPreviewEntryIsReleasedOnDecodedReplacement();
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

KiriView::ImagePresentationRenderProjection visibleProjection()
{
    KiriView::ImagePresentationRenderProjection projection;
    projection.visible = true;
    projection.displaySize = QSizeF(8.0, 4.0);
    projection.visibleItemRect = QRectF(0.0, 0.0, 8.0, 4.0);
    projection.maximumTextureSize = KiriView::fallbackTextureSizeMax;
    return projection;
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

QTEST_GUILESS_MAIN(TestImagePageSurfaceController)

#include "test_imagepagesurfacecontroller.moc"
