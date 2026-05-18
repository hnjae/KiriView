// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagesurface.h"

#include <QObject>
#include <QTest>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

namespace {
class HugeByteCostTileSource final : public KiriView::ImageTileSource
{
public:
    QSize imageSize() const override { return QSize(1, 1); }

    std::optional<KiriView::DecodedTile> decodeTile(
        const KiriView::TileRequest &, QString *) const override
    {
        return std::nullopt;
    }

    QImage decodeBlockingDisplayImage(int, QString *) const override
    {
        return KiriView::TestSupport::testImage();
    }

    qsizetype byteCost() const override { return std::numeric_limits<qsizetype>::max(); }
};
}

class TestImageSurface : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void staticImagePayloadReportsByteCostWithinBudget();
    void staticImagePayloadByteCostSaturatesOversizedSources();
    void tileCacheByteBudgetUsesFullDecodeLimitAndSystemMemoryCap();
    void fullImageSurfacePolicyRequiresMatchingPreviewWithinTextureLimit();
    void displayedImageSurfaceExposesOnlyActivePayload();
};

void TestImageSurface::staticImagePayloadReportsByteCostWithinBudget()
{
    const KiriView::StaticImagePayload image
        = KiriView::TestSupport::staticDecodedTestImage(KiriView::TestSupport::testImage(2, 1))
              .staticImage;
    const qsizetype byteCost = image.byteCost();

    const std::optional<qsizetype> acceptedByteCost = image.byteCostWithinBudget(byteCost);
    QVERIFY(acceptedByteCost.has_value());
    QCOMPARE(*acceptedByteCost, byteCost);
    QVERIFY(!image.byteCostWithinBudget(byteCost - 1).has_value());
    QVERIFY(!image.byteCostWithinBudget(0).has_value());
    QVERIFY(!KiriView::StaticImagePayload().byteCostWithinBudget(byteCost).has_value());
}

void TestImageSurface::staticImagePayloadByteCostSaturatesOversizedSources()
{
    const KiriView::StaticImagePayload image {
        std::make_shared<HugeByteCostTileSource>(),
        KiriView::TestSupport::testImage(),
        {},
    };

    QCOMPARE(image.byteCost(), std::numeric_limits<qsizetype>::max());
    QVERIFY(!image.byteCostWithinBudget(std::numeric_limits<qsizetype>::max() - 1).has_value());
    const std::optional<qsizetype> byteCost
        = image.byteCostWithinBudget(std::numeric_limits<qsizetype>::max());
    QVERIFY(byteCost.has_value());
    QCOMPARE(*byteCost, std::numeric_limits<qsizetype>::max());
}

void TestImageSurface::tileCacheByteBudgetUsesFullDecodeLimitAndSystemMemoryCap()
{
    constexpr qsizetype preferredByteBudget = KiriView::imageFullDecodeFallbackByteLimit;

    QCOMPARE(
        KiriView::StaticTileSurface::tileCacheByteBudgetForSystemMemory(0), preferredByteBudget);
    QCOMPARE(KiriView::StaticTileSurface::tileCacheByteBudgetForSystemMemory(preferredByteBudget),
        preferredByteBudget / 16);
    QCOMPARE(
        KiriView::StaticTileSurface::tileCacheByteBudgetForSystemMemory(preferredByteBudget * 32),
        preferredByteBudget);
    QVERIFY(KiriView::StaticTileSurface::defaultTileCacheByteBudget() > 0);
    QVERIFY(KiriView::StaticTileSurface::defaultTileCacheByteBudget() <= preferredByteBudget);
}

void TestImageSurface::fullImageSurfacePolicyRequiresMatchingPreviewWithinTextureLimit()
{
    const QImage sourceImage = KiriView::TestSupport::testImage(512, 256);
    const QImage matchingPreview = KiriView::TestSupport::testImage(512, 256);
    const QImage scaledPreview = KiriView::TestSupport::testImage(256, 128);

    QVERIFY(KiriView::staticImageFitsFullImageSurface(
        KiriView::TestSupport::staticTestImagePayload(sourceImage, matchingPreview), 512));
    QVERIFY(!KiriView::staticImageFitsFullImageSurface(
        KiriView::TestSupport::staticTestImagePayload(sourceImage, scaledPreview), 512));
    QVERIFY(!KiriView::staticImageFitsFullImageSurface(
        KiriView::TestSupport::staticTestImagePayload(sourceImage, matchingPreview), 511));
    QVERIFY(!KiriView::staticImageFitsFullImageSurface(KiriView::StaticImagePayload {}, 512));
}

void TestImageSurface::displayedImageSurfaceExposesOnlyActivePayload()
{
    const QImage legacyImage = KiriView::TestSupport::testImage(2, 1);
    const KiriView::DisplayedImageSurface legacySurface(
        KiriView::LegacyFrameSurface { legacyImage });

    QVERIFY(!legacySurface.isNull());
    QCOMPARE(legacySurface.imageSize(), QSize(2, 1));
    QVERIFY(legacySurface.legacyFrameSurface() != nullptr);
    QVERIFY(legacySurface.staticTileSurface() == nullptr);

    KiriView::StaticImagePayload staticImage
        = KiriView::TestSupport::staticDecodedTestImage(KiriView::TestSupport::testImage(3, 2))
              .staticImage;
    const KiriView::DisplayedImageSurface staticSurface(
        KiriView::StaticTileSurface { std::move(staticImage) });

    QVERIFY(!staticSurface.isNull());
    QCOMPARE(staticSurface.imageSize(), QSize(3, 2));
    QVERIFY(staticSurface.legacyFrameSurface() == nullptr);
    QVERIFY(staticSurface.staticTileSurface() != nullptr);
}

QTEST_GUILESS_MAIN(TestImageSurface)

#include "test_imagesurface.moc"
