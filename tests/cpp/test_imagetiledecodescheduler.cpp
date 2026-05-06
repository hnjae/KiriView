// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagetiledecodescheduler.h"

#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <cstddef>
#include <memory>
#include <utility>

namespace {
using KiriView::TestSupport::testImage;

std::shared_ptr<KiriView::DisplayedImageSurface> testSurface(
    const QSize &imageSize, KiriView::StaticImageDisplayHints displayHints = {})
{
    const QImage image = testImage(imageSize);
    KiriView::StaticImagePayload payload {
        std::make_shared<KiriView::TestSupport::TestImageTileSource>(image),
        testImage(QSize(512, 512)),
        displayHints,
    };
    return std::make_shared<KiriView::DisplayedImageSurface>(
        KiriView::StaticTileSurface { std::move(payload) });
}

std::size_t tileCount(const std::shared_ptr<KiriView::DisplayedImageSurface> &surface)
{
    auto *staticSurface = surface == nullptr ? nullptr : surface->staticTileSurface();
    return staticSurface == nullptr ? 0 : staticSurface->tiles().size();
}

void insertTile(
    const std::shared_ptr<KiriView::DisplayedImageSurface> &surface, KiriView::DecodedTile tile)
{
    auto *staticSurface = surface->staticTileSurface();
    QVERIFY(staticSurface != nullptr);
    QVERIFY(staticSurface->insertTile(std::move(tile)));
}
}

class TestImageTileDecodeScheduler : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void visibleTilesAreDecodedThroughCallback();
    void firstDisplayHintDefersTileDecodeUntilDisplayNeedsMoreDetail();
};

void TestImageTileDecodeScheduler::visibleTilesAreDecodedThroughCallback()
{
    const auto surface = testSurface(QSize(2048, 1024));
    KiriView::ImageTileDecodeScheduler scheduler(
        nullptr, [surface](KiriView::DecodedTile tile) { insertTile(surface, std::move(tile)); });

    scheduler.schedule(surface, QSizeF(2048.0, 1024.0), QRectF(0.0, 0.0, 512.0, 512.0),
        KiriView::ImageDocumentRenderContext {});

    QVERIFY(tileCount(surface) > std::size_t(0));
}

void TestImageTileDecodeScheduler::firstDisplayHintDefersTileDecodeUntilDisplayNeedsMoreDetail()
{
    const auto surface = testSurface(QSize(2048, 2048), KiriView::StaticImageDisplayHints { 0.25 });
    KiriView::ImageTileDecodeScheduler scheduler(
        nullptr, [surface](KiriView::DecodedTile tile) { insertTile(surface, std::move(tile)); });

    scheduler.schedule(surface, QSizeF(512.0, 512.0), QRectF(0.0, 0.0, 512.0, 512.0),
        KiriView::ImageDocumentRenderContext {});

    QCOMPARE(tileCount(surface), std::size_t(0));

    scheduler.schedule(surface, QSizeF(2048.0, 2048.0), QRectF(0.0, 0.0, 512.0, 512.0),
        KiriView::ImageDocumentRenderContext {});

    QVERIFY(tileCount(surface) > std::size_t(0));
}

QTEST_GUILESS_MAIN(TestImageTileDecodeScheduler)

#include "test_imagetiledecodescheduler.moc"
