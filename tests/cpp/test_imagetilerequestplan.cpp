// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rendering/imagetilerequestplan.h"

#include "image_test_support.h"
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
#include <utility>

namespace {
constexpr qsizetype testTileCacheByteBudget = KiriView::imageFullDecodeFallbackByteLimit;

using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

KiriView::DecodedTile decodedTileForRequest(const KiriView::TileRequest &request)
{
    QImage image(request.textureLevelRect.size(), QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    return KiriView::DecodedTile {
        request.key,
        request.levelSize,
        request.levelRect,
        request.textureLevelRect,
        std::move(image),
        request.displaySourceRect,
        request.displaySourceRectF,
    };
}

KiriView::StaticTileSurface rasterTileSurface(
    const QSize &imageSize, KiriView::StaticImageDisplayHints displayHints = {})
{
    return KiriView::StaticTileSurface {
        staticTestImagePayload(testImage(imageSize), testImage(QSize(512, 512)), displayHints),
        testTileCacheByteBudget,
    };
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

KiriView::StaticTileSurface svgTileSurface(const QSize &imageSize)
{
    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(svgData(imageSize), &errorString);
    Q_ASSERT(source != nullptr);

    return KiriView::StaticTileSurface {
        KiriView::StaticImagePayload {
            std::move(source),
            testImage(imageSize),
            {},
        },
        testTileCacheByteBudget,
    };
}

KiriView::ImageTileRequestPlan planForSurface(const KiriView::StaticTileSurface &surface,
    const QSizeF &displaySize, const QRectF &visibleItemRect, qreal devicePixelRatio = 1.0,
    int rotationDegrees = 0, bool resolutionIndependent = false,
    KiriView::ImageTileDecodeExclusions exclusions = {})
{
    return KiriView::planImageTileRequests(KiriView::ImageTileRequestPlanInput {
        &surface,
        KiriView::ImageSurfaceDrawContext {
            QRectF(0.0, 0.0, displaySize.width(), displaySize.height()),
            displaySize,
            visibleItemRect,
            devicePixelRatio,
            rotationDegrees,
        },
        std::move(exclusions),
        resolutionIndependent,
    });
}

bool containsRequestForKey(const KiriView::ImageTileRequestPlan &plan, const KiriView::TileKey &key)
{
    return std::any_of(plan.missingRequests.cbegin(), plan.missingRequests.cend(),
        [&key](const KiriView::TileRequest &request) { return request.key == key; });
}
}

class TestImageTileRequestPlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rasterFirstDisplayHintSuppressesOnlyDecodeRequests();
    void existingPendingAndFailedRasterTilesAreFiltered();
    void resolutionIndependentRequestsUseSourceVisibilityForRotatedViews();
};

void TestImageTileRequestPlan::rasterFirstDisplayHintSuppressesOnlyDecodeRequests()
{
    const KiriView::StaticTileSurface surface
        = rasterTileSurface(QSize(2048, 2048), KiriView::StaticImageDisplayHints { 0.25 });

    const KiriView::ImageTileRequestPlan plan
        = planForSurface(surface, QSizeF(512.0, 512.0), QRectF(0.0, 0.0, 512.0, 512.0));

    QVERIFY(!plan.hasRequests());
    QVERIFY(plan.source == nullptr);
    QCOMPARE(plan.activeTileLayer.level, 2);
}

void TestImageTileRequestPlan::existingPendingAndFailedRasterTilesAreFiltered()
{
    KiriView::StaticTileSurface surface = rasterTileSurface(QSize(2048, 1024));
    const KiriView::ImageTileRequestPlan initialPlan
        = planForSurface(surface, QSizeF(2048.0, 1024.0), QRectF(0.0, 0.0, 2048.0, 1024.0));
    QVERIFY(initialPlan.hasRequests());
    QVERIFY(initialPlan.missingRequests.size() >= std::size_t(3));

    const KiriView::TileKey existingKey = initialPlan.missingRequests.at(0).key;
    const KiriView::TileKey pendingKey = initialPlan.missingRequests.at(1).key;
    const KiriView::TileKey failedKey = initialPlan.missingRequests.at(2).key;
    QVERIFY(surface.insertTile(decodedTileForRequest(initialPlan.missingRequests.at(0))));

    KiriView::ImageTileDecodeExclusions exclusions;
    exclusions.pendingTileKeys.insert(pendingKey);
    exclusions.failedTileKeys.insert(failedKey);

    const KiriView::ImageTileRequestPlan filteredPlan
        = planForSurface(surface, QSizeF(2048.0, 1024.0), QRectF(0.0, 0.0, 2048.0, 1024.0), 1.0, 0,
            false, std::move(exclusions));

    QVERIFY(!containsRequestForKey(filteredPlan, existingKey));
    QVERIFY(!containsRequestForKey(filteredPlan, pendingKey));
    QVERIFY(!containsRequestForKey(filteredPlan, failedKey));
    QCOMPARE(filteredPlan.missingRequests.size(), initialPlan.missingRequests.size() - 3);
}

void TestImageTileRequestPlan::resolutionIndependentRequestsUseSourceVisibilityForRotatedViews()
{
    const KiriView::StaticTileSurface surface = svgTileSurface(QSize(2048, 1024));

    const KiriView::ImageTileRequestPlan plan = planForSurface(
        surface, QSizeF(1024.0, 2048.0), QRectF(0.0, 512.0, 512.0, 512.0), 1.0, 90, true);

    QVERIFY(plan.hasRequests());
    QCOMPARE(plan.missingRequests.front().key.level, 0);
    QCOMPARE(plan.missingRequests.front().key.x, 1);
    QCOMPARE(plan.missingRequests.front().key.y, 1);
    QVERIFY(plan.missingRequests.front().key.scaleBucket > 0);
    for (const KiriView::TileRequest &request : plan.missingRequests) {
        QVERIFY(!request.displaySourceRect.isEmpty());
    }
}

QTEST_GUILESS_MAIN(TestImageTileRequestPlan)

#include "test_imagetilerequestplan.moc"
