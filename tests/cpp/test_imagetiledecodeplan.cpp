// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "rendering/imagetiledecodeplan.h"
#include "rendering/svgtilesource.h"

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QTest>
#include <Qt>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

namespace {
constexpr qsizetype testTileCacheByteBudget = KiriView::imageFullDecodeFallbackByteLimit;
constexpr int testMaximumTextureSize = 4096;

using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

std::shared_ptr<KiriView::DisplayedImageSurface> testSurface(
    const QSize &imageSize, KiriView::StaticImageDisplayHints displayHints = {})
{
    const QImage image = testImage(imageSize);
    KiriView::StaticImagePayload payload
        = staticTestImagePayload(image, testImage(QSize(512, 512)), displayHints);
    return std::make_shared<KiriView::DisplayedImageSurface>(
        KiriView::StaticTileSurface { std::move(payload), testTileCacheByteBudget });
}

QByteArray svgData(const QSize &imageSize)
{
    return QStringLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%1\" height=\"%2\">"
                          "<rect width=\"%1\" height=\"%2\" fill=\"red\"/>"
                          "</svg>")
        .arg(imageSize.width())
        .arg(imageSize.height())
        .toUtf8();
}

std::shared_ptr<KiriView::DisplayedImageSurface> svgSurface(const QSize &imageSize)
{
    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(svgData(imageSize), &errorString);
    Q_ASSERT(source != nullptr);

    KiriView::StaticImagePayload payload {
        std::move(source),
        testImage(imageSize),
        {},
    };
    return std::make_shared<KiriView::DisplayedImageSurface>(
        KiriView::StaticTileSurface { std::move(payload), testTileCacheByteBudget });
}

bool containsRequestForKey(const KiriView::ImageTileDecodePlan &plan, const KiriView::TileKey &key)
{
    return std::any_of(plan.requests.cbegin(), plan.requests.cend(),
        [&key](const KiriView::TileRequest &request) { return request.key == key; });
}

void insertDecodedTile(const KiriView::ImageTileDecodePlan &plan,
    const std::shared_ptr<KiriView::DisplayedImageSurface> &surface,
    const KiriView::TileRequest &request)
{
    QString errorString;
    std::optional<KiriView::DecodedTile> tile = plan.source->decodeTile(request, &errorString);
    QVERIFY2(tile.has_value(), qPrintable(errorString));

    auto *staticSurface = surface->staticTileSurface();
    QVERIFY(staticSurface != nullptr);
    QVERIFY(staticSurface->insertTile(std::move(*tile)));
}

void insertSyntheticDecodedTiles(const KiriView::ImageTileDecodePlan &plan,
    const std::shared_ptr<KiriView::DisplayedImageSurface> &surface)
{
    auto *staticSurface = surface->staticTileSurface();
    QVERIFY(staticSurface != nullptr);

    for (const KiriView::TileRequest &request : plan.requests) {
        QImage image(request.textureLevelRect.size(), QImage::Format_RGBA8888_Premultiplied);
        image.fill(Qt::transparent);
        QVERIFY(staticSurface->insertTile(KiriView::DecodedTile {
            request.key,
            request.levelSize,
            request.levelRect,
            request.textureLevelRect,
            std::move(image),
            request.displaySourceRect,
            request.displaySourceRectF,
        }));
    }
}
}

class TestImageTileDecodePlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void visibleStaticTilesBecomeDecodeRequests();
    void firstDisplayHintSuppressesUnneededRequests();
    void existingPendingAndFailedTilesAreFiltered();
    void svgScaleBucketKeysChangeOnlyAcrossBucketBoundary();
};

void TestImageTileDecodePlan::visibleStaticTilesBecomeDecodeRequests()
{
    const auto surface = testSurface(QSize(2048, 1024));

    const KiriView::ImageTileDecodePlan plan
        = KiriView::imageTileDecodePlan(surface, QSizeF(2048.0, 1024.0),
            QRectF(0.0, 0.0, 1024.0, 1024.0), KiriView::ImageDocumentRenderContext {}, 0, {});

    QVERIFY(plan.source != nullptr);
    QVERIFY(!plan.requests.empty());
    for (const KiriView::TileRequest &request : plan.requests) {
        QVERIFY(!request.textureLevelRect.isEmpty());
        QVERIFY(!request.sourceRect.isEmpty());
    }
}

void TestImageTileDecodePlan::firstDisplayHintSuppressesUnneededRequests()
{
    const auto surface = testSurface(QSize(2048, 2048), KiriView::StaticImageDisplayHints { 0.25 });

    const KiriView::ImageTileDecodePlan suppressedPlan
        = KiriView::imageTileDecodePlan(surface, QSizeF(512.0, 512.0),
            QRectF(0.0, 0.0, 512.0, 512.0), KiriView::ImageDocumentRenderContext {}, 0, {});
    QVERIFY(suppressedPlan.isEmpty());

    const KiriView::ImageTileDecodePlan detailedPlan
        = KiriView::imageTileDecodePlan(surface, QSizeF(2048.0, 2048.0),
            QRectF(0.0, 0.0, 512.0, 512.0), KiriView::ImageDocumentRenderContext {}, 0, {});
    QVERIFY(!detailedPlan.isEmpty());
}

void TestImageTileDecodePlan::existingPendingAndFailedTilesAreFiltered()
{
    const auto surface = testSurface(QSize(2048, 1024));
    const KiriView::ImageTileDecodePlan initialPlan
        = KiriView::imageTileDecodePlan(surface, QSizeF(2048.0, 1024.0),
            QRectF(0.0, 0.0, 2048.0, 1024.0), KiriView::ImageDocumentRenderContext {}, 0, {});
    QVERIFY(initialPlan.requests.size() >= 3);

    const KiriView::TileKey existingKey = initialPlan.requests.at(0).key;
    const KiriView::TileKey pendingKey = initialPlan.requests.at(1).key;
    const KiriView::TileKey failedKey = initialPlan.requests.at(2).key;
    insertDecodedTile(initialPlan, surface, initialPlan.requests.at(0));

    KiriView::ImageTileDecodeExclusions exclusions;
    exclusions.pendingTileKeys.insert(pendingKey);
    exclusions.failedTileKeys.insert(failedKey);

    const KiriView::ImageTileDecodePlan filteredPlan = KiriView::imageTileDecodePlan(surface,
        QSizeF(2048.0, 1024.0), QRectF(0.0, 0.0, 2048.0, 1024.0),
        KiriView::ImageDocumentRenderContext {}, 0, exclusions);

    QVERIFY(!containsRequestForKey(filteredPlan, existingKey));
    QVERIFY(!containsRequestForKey(filteredPlan, pendingKey));
    QVERIFY(!containsRequestForKey(filteredPlan, failedKey));
    QCOMPARE(filteredPlan.requests.size(), initialPlan.requests.size() - 3);
}

void TestImageTileDecodePlan::svgScaleBucketKeysChangeOnlyAcrossBucketBoundary()
{
    const auto surface = svgSurface(QSize(1000, 1000));
    const KiriView::ImageDocumentRenderContext context { 1.0, testMaximumTextureSize };

    const KiriView::ImageTileDecodePlan initialPlan = KiriView::imageTileDecodePlan(
        surface, QSizeF(1000.0, 1000.0), QRectF(0.0, 0.0, 1000.0, 1000.0), context, 0, {});
    QVERIFY(!initialPlan.isEmpty());
    for (const KiriView::TileRequest &request : initialPlan.requests) {
        QCOMPARE(request.key.level, 0);
        QCOMPARE(request.key.scaleBucket, 1);
        QCOMPARE(request.levelSize, QSize(1500, 1500));
        QVERIFY(!request.displaySourceRect.isEmpty());
    }

    insertSyntheticDecodedTiles(initialPlan, surface);

    const KiriView::ImageTileDecodePlan sameBucketPlan = KiriView::imageTileDecodePlan(
        surface, QSizeF(1200.0, 1200.0), QRectF(0.0, 0.0, 1200.0, 1200.0), context, 0, {});
    QVERIFY(sameBucketPlan.isEmpty());

    const KiriView::ImageTileDecodePlan nextBucketPlan = KiriView::imageTileDecodePlan(
        surface, QSizeF(1600.0, 1600.0), QRectF(0.0, 0.0, 1600.0, 1600.0), context, 0, {});
    QVERIFY(!nextBucketPlan.isEmpty());
    for (const KiriView::TileRequest &request : nextBucketPlan.requests) {
        QCOMPARE(request.key.level, 0);
        QCOMPARE(request.key.scaleBucket, 2);
        QCOMPARE(request.levelSize, QSize(2250, 2250));
    }
}

QTEST_GUILESS_MAIN(TestImageTileDecodePlan)

#include "test_imagetiledecodeplan.moc"
