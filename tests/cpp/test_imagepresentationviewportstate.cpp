// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepresentationviewportstate.h"

#include "rendering/imagerendering.h"

#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <algorithm>
#include <vector>

namespace {
bool containsChange(
    const std::vector<KiriView::ImageDocumentChange> &changes, KiriView::ImageDocumentChange change)
{
    return std::find(changes.cbegin(), changes.cend(), change) != changes.cend();
}
}

class TestImagePresentationViewportState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void displayedImageSizeUpdatesZoomAndTilePlan();
    void rotationOwnsLogicalImageSizeAndPlan();
    void redundantSourceAndRotationUpdatesReturnEmptyPlans();
    void visibleRectPlansTileRefreshWithoutDuplicatingState();
    void renderContextRefreshPlansForcedTileRefresh();
    void renderContextGenerationRefreshInvalidatesTileLifecycle();
};

void TestImagePresentationViewportState::displayedImageSizeUpdatesZoomAndTilePlan()
{
    KiriView::ImagePresentationViewportState state([]() {
        return KiriView::ImageDocumentRenderContext { 1.0, KiriView::fallbackTextureSizeMax };
    });
    state.setViewportSize(QSizeF(400.0, 200.0));

    const KiriView::ImagePresentationViewportPlan plan
        = state.setDisplayedImageSize(QSize(800, 400));

    QCOMPARE(state.imageSize(), QSize(800, 400));
    QCOMPARE(state.displaySize(), QSizeF(400.0, 200.0));
    QVERIFY(containsChange(plan.changes, KiriView::ImageDocumentChange::ImageSize));
    QVERIFY(containsChange(plan.changes, KiriView::ImageDocumentChange::DisplaySize));
    QVERIFY(containsChange(plan.changes, KiriView::ImageDocumentChange::Repaint));
    QVERIFY(plan.scheduleVisibleTileDecode);
}

void TestImagePresentationViewportState::rotationOwnsLogicalImageSizeAndPlan()
{
    KiriView::ImagePresentationViewportState state([]() {
        return KiriView::ImageDocumentRenderContext { 1.0, KiriView::fallbackTextureSizeMax };
    });
    state.setViewportSize(QSizeF(100.0, 100.0));
    state.setDisplayedImageSize(QSize(20, 10));

    const KiriView::ImagePresentationViewportPlan plan = state.rotateClockwise();

    QCOMPARE(state.rotationDegrees(), 90);
    QCOMPARE(state.imageSize(), QSize(10, 20));
    QVERIFY(containsChange(plan.changes, KiriView::ImageDocumentChange::Rotation));
    QVERIFY(containsChange(plan.changes, KiriView::ImageDocumentChange::ImageSize));
    QVERIFY(containsChange(plan.changes, KiriView::ImageDocumentChange::Repaint));
    QVERIFY(plan.scheduleVisibleTileDecode);

    const KiriView::ImagePresentationViewportPlan counterclockwisePlan
        = state.rotateCounterclockwise();

    QCOMPARE(state.rotationDegrees(), 0);
    QCOMPARE(state.imageSize(), QSize(20, 10));
    QVERIFY(containsChange(counterclockwisePlan.changes, KiriView::ImageDocumentChange::Rotation));
    QVERIFY(containsChange(counterclockwisePlan.changes, KiriView::ImageDocumentChange::ImageSize));
    QVERIFY(counterclockwisePlan.scheduleVisibleTileDecode);
}

void TestImagePresentationViewportState::redundantSourceAndRotationUpdatesReturnEmptyPlans()
{
    KiriView::ImagePresentationViewportState state([]() {
        return KiriView::ImageDocumentRenderContext { 1.0, KiriView::fallbackTextureSizeMax };
    });
    state.setViewportSize(QSizeF(100.0, 100.0));
    state.setDisplayedImageSize(QSize(20, 10));
    state.rotateClockwise();

    KiriView::ImagePresentationViewportPlan plan = state.setDisplayedImageSize(QSize(20, 10));

    QVERIFY(plan.changes.empty());
    QVERIFY(!plan.scheduleVisibleTileDecode);

    plan = state.rotateClockwise();

    QCOMPARE(state.rotationDegrees(), 180);
    QCOMPARE(state.imageSize(), QSize(20, 10));
    QVERIFY(containsChange(plan.changes, KiriView::ImageDocumentChange::Rotation));
    QVERIFY(containsChange(plan.changes, KiriView::ImageDocumentChange::ImageSize));

    plan = state.resetRotation();
    QVERIFY(containsChange(plan.changes, KiriView::ImageDocumentChange::Rotation));
    QVERIFY(!containsChange(plan.changes, KiriView::ImageDocumentChange::ImageSize));
    QCOMPARE(state.rotationDegrees(), 0);

    plan = state.resetRotation();
    QVERIFY(plan.changes.empty());
    QVERIFY(!plan.scheduleVisibleTileDecode);
}

void TestImagePresentationViewportState::visibleRectPlansTileRefreshWithoutDuplicatingState()
{
    KiriView::ImagePresentationViewportState state;

    KiriView::ImagePresentationViewportPlan plan
        = state.setVisibleItemRect(QRectF(1.0, 2.0, 3.0, 4.0));

    QCOMPARE(state.visibleItemRect(), QRectF(1.0, 2.0, 3.0, 4.0));
    QCOMPARE(plan.changes.size(), std::size_t(2));
    QCOMPARE(plan.changes.front(), KiriView::ImageDocumentChange::VisibleItemRect);
    QVERIFY(containsChange(plan.changes, KiriView::ImageDocumentChange::RenderFrame));
    QVERIFY(plan.scheduleVisibleTileDecode);

    plan = state.setVisibleItemRect(QRectF(1.0, 2.0, 3.0, 4.0));

    QVERIFY(plan.changes.empty());
    QVERIFY(!plan.scheduleVisibleTileDecode);
}

void TestImagePresentationViewportState::renderContextRefreshPlansForcedTileRefresh()
{
    qreal devicePixelRatio = 1.0;
    KiriView::ImagePresentationViewportState state([&devicePixelRatio]() {
        return KiriView::ImageDocumentRenderContext {
            devicePixelRatio,
            KiriView::fallbackTextureSizeMax,
        };
    });
    state.setViewportSize(QSizeF(128.0, 128.0));
    state.setDisplayedImageSize(QSize(256, 256));

    devicePixelRatio = 2.0;
    const KiriView::ImagePresentationViewportPlan plan = state.updateRenderContext();

    QCOMPARE(state.renderContext().devicePixelRatio, 2.0);
    QVERIFY(containsChange(plan.changes, KiriView::ImageDocumentChange::RenderFrame));
    QVERIFY(!plan.invalidateTiles);
    QVERIFY(plan.scheduleVisibleTileDecode);
}

void TestImagePresentationViewportState::renderContextGenerationRefreshInvalidatesTileLifecycle()
{
    quint64 generation = 1;
    KiriView::ImagePresentationViewportState state([&generation]() {
        return KiriView::ImageDocumentRenderContext {
            1.0,
            KiriView::fallbackTextureSizeMax,
            generation,
        };
    });
    state.setViewportSize(QSizeF(128.0, 128.0));
    state.setDisplayedImageSize(QSize(256, 256));

    generation = 2;
    const KiriView::ImagePresentationViewportPlan plan = state.updateRenderContext();

    QCOMPARE(state.renderContext().generation, quint64(2));
    QVERIFY(plan.invalidateTiles);
    QVERIFY(containsChange(plan.changes, KiriView::ImageDocumentChange::RenderFrame));
    QVERIFY(plan.scheduleVisibleTileDecode);
}

QTEST_GUILESS_MAIN(TestImagePresentationViewportState)

#include "test_imagepresentationviewportstate.moc"
