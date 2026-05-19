// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagetiledecodescheduler.h"

#include <QImage>
#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QTest>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

namespace {
constexpr qsizetype testTileCacheByteBudget = KiriView::imageFullDecodeFallbackByteLimit;

using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

class FailingTileSource final : public KiriView::ImageTileSource
{
public:
    FailingTileSource(QSize imageSize, int &decodeCount)
        : m_imageSize(imageSize)
        , m_decodeCount(&decodeCount)
    {
    }

    QSize imageSize() const override { return m_imageSize; }

    std::optional<KiriView::DecodedTile> decodeTile(
        const KiriView::TileRequest &, QString *) const override
    {
        ++*m_decodeCount;
        return std::nullopt;
    }

    QImage decodeBlockingDisplayImage(int, QString *) const override { return {}; }

    qsizetype byteCost() const override { return 0; }

private:
    QSize m_imageSize;
    int *m_decodeCount = nullptr;
};

std::shared_ptr<KiriView::DisplayedImageSurface> testSurface(
    const QSize &imageSize, KiriView::StaticImageDisplayHints displayHints = {})
{
    const QImage image = testImage(imageSize);
    KiriView::StaticImagePayload payload
        = staticTestImagePayload(image, testImage(QSize(512, 512)), displayHints);
    return std::make_shared<KiriView::DisplayedImageSurface>(
        KiriView::StaticTileSurface { std::move(payload), testTileCacheByteBudget });
}

std::shared_ptr<KiriView::DisplayedImageSurface> failingSurface(
    const QSize &imageSize, int &decodeCount)
{
    KiriView::StaticImagePayload payload {
        std::make_shared<FailingTileSource>(imageSize, decodeCount),
        testImage(QSize(512, 512)),
        {},
    };
    return std::make_shared<KiriView::DisplayedImageSurface>(
        KiriView::StaticTileSurface { std::move(payload), testTileCacheByteBudget });
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
    void failedTileKeysDoNotLeakAcrossSurfaces();
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

void TestImageTileDecodeScheduler::failedTileKeysDoNotLeakAcrossSurfaces()
{
    int firstDecodeCount = 0;
    int secondDecodeCount = 0;
    const auto firstSurface = failingSurface(QSize(2048, 1024), firstDecodeCount);
    const auto secondSurface = failingSurface(QSize(2048, 1024), secondDecodeCount);
    KiriView::ImageTileDecodeScheduler scheduler(nullptr, {});

    scheduler.schedule(firstSurface, QSizeF(2048.0, 1024.0), QRectF(0.0, 0.0, 512.0, 512.0),
        KiriView::ImageDocumentRenderContext {});
    QVERIFY(firstDecodeCount > 0);
    const int failedDecodeCount = firstDecodeCount;

    scheduler.schedule(firstSurface, QSizeF(2048.0, 1024.0), QRectF(0.0, 0.0, 512.0, 512.0),
        KiriView::ImageDocumentRenderContext {});
    QCOMPARE(firstDecodeCount, failedDecodeCount);

    scheduler.schedule(secondSurface, QSizeF(2048.0, 1024.0), QRectF(0.0, 0.0, 512.0, 512.0),
        KiriView::ImageDocumentRenderContext {});
    QVERIFY(secondDecodeCount > 0);
}

QTEST_GUILESS_MAIN(TestImageTileDecodeScheduler)

#include "test_imagetiledecodescheduler.moc"
