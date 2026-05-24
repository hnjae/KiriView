// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepresentationviewportcontroller.h"

#include "image_test_support.h"
#include "rendering/imagerendering.h"

#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace {
constexpr qsizetype testTileCacheByteBudget = KiriView::imageFullDecodeFallbackByteLimit;

using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

std::shared_ptr<KiriView::DisplayedImageSurface> testSurface(const QSize &imageSize)
{
    const QImage image = testImage(imageSize);
    KiriView::StaticImagePayload payload
        = staticTestImagePayload(image, testImage(QSize(512, 512)));
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

bool containsChange(
    const std::vector<KiriView::ImageDocumentChange> &changes, KiriView::ImageDocumentChange change)
{
    return std::find(changes.cbegin(), changes.cend(), change) != changes.cend();
}
}

class TestImagePresentationViewportController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rotationOwnsLogicalImageSizeAndNotifications();
    void visibleRectSchedulesTilesThroughOwnedScheduler();
};

void TestImagePresentationViewportController::rotationOwnsLogicalImageSizeAndNotifications()
{
    std::vector<KiriView::ImageDocumentChange> changes;
    KiriView::ImagePresentationViewportController controller(
        nullptr,
        []() {
            return KiriView::ImageDocumentRenderContext {
                1.0,
                KiriView::fallbackTextureSizeMax,
            };
        },
        {}, {}, [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });

    controller.setViewportSize(QSizeF(100.0, 100.0));
    controller.setDisplayedImageSize(QSize(20, 10));

    changes.clear();
    controller.rotateClockwise();

    QCOMPARE(controller.rotationDegrees(), 90);
    QCOMPARE(controller.imageSize(), QSize(10, 20));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::Rotation));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::ImageSize));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::Repaint));
}

void TestImagePresentationViewportController::visibleRectSchedulesTilesThroughOwnedScheduler()
{
    const auto surface = testSurface(QSize(2048, 1024));
    std::vector<KiriView::ImageDocumentChange> changes;
    KiriView::ImagePresentationViewportController controller(
        nullptr,
        []() {
            return KiriView::ImageDocumentRenderContext {
                1.0,
                KiriView::fallbackTextureSizeMax,
            };
        },
        [surface]() { return surface; },
        [surface](KiriView::DecodedTile tile) { insertTile(surface, std::move(tile)); },
        [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });

    controller.setViewportSize(QSizeF(2048.0, 1024.0));
    controller.setDisplayedImageSize(QSize(2048, 1024));
    changes.clear();

    controller.setVisibleItemRect(QRectF(0.0, 0.0, 512.0, 512.0));

    QTRY_VERIFY(tileCount(surface) > std::size_t(0));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::VisibleItemRect));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::RenderFrame));
}

QTEST_GUILESS_MAIN(TestImagePresentationViewportController)

#include "test_imagepresentationviewportcontroller.moc"
