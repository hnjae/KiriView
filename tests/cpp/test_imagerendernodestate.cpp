// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagerendernodestate.h"

#include <QObject>
#include <QRectF>
#include <QTest>

class TestImageRenderNodeState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void surfaceUpdateOwnsRevisionAndPointerInvalidation();
    void targetGeometryOnlyRequestsDrawGeometrySync();
    void drawGeometrySyncFailurePromotesTextureRebuild();
    void rotationIsNormalizedBeforeComparison();
    void resetUploadedResourcesKeepsPublicInputs();
};

namespace {
void expectTextureUpdatePlan(const KiriView::ImageRenderNodeState &state,
    KiriView::ImageRenderNodeTextureUpdatePlan expectedPlan)
{
    QCOMPARE(static_cast<int>(state.textureUpdatePlan()), static_cast<int>(expectedPlan));
}
}

void TestImageRenderNodeState::surfaceUpdateOwnsRevisionAndPointerInvalidation()
{
    KiriView::ImageRenderNodeState state;

    KiriView::ImageRenderNodeSurfaceUpdate update = state.setSurface(false, 10);
    QVERIFY(update.acceptSurface);
    expectTextureUpdatePlan(state, KiriView::ImageRenderNodeTextureUpdatePlan::RebuildTextures);

    state.markTexturesUploaded();
    expectTextureUpdatePlan(state, KiriView::ImageRenderNodeTextureUpdatePlan::ReuseTextures);

    update = state.setSurface(true, 10);
    QVERIFY(!update.acceptSurface);
    expectTextureUpdatePlan(state, KiriView::ImageRenderNodeTextureUpdatePlan::ReuseTextures);

    update = state.setSurface(true, 11);
    QVERIFY(update.acceptSurface);
    expectTextureUpdatePlan(state, KiriView::ImageRenderNodeTextureUpdatePlan::RebuildTextures);

    state.markTexturesUploaded();
    expectTextureUpdatePlan(state, KiriView::ImageRenderNodeTextureUpdatePlan::ReuseTextures);

    update = state.setSurface(false, 11);
    QVERIFY(update.acceptSurface);
    expectTextureUpdatePlan(state, KiriView::ImageRenderNodeTextureUpdatePlan::RebuildTextures);
}

void TestImageRenderNodeState::targetGeometryOnlyRequestsDrawGeometrySync()
{
    KiriView::ImageRenderNodeState state;
    state.setSurface(false, 7);
    state.markTexturesUploaded();

    QVERIFY(state.setTargetRect(QRectF(10.0, 20.0, 30.0, 40.0)));
    expectTextureUpdatePlan(
        state, KiriView::ImageRenderNodeTextureUpdatePlan::SynchronizeDrawGeometry);
    QCOMPARE(state.targetRect(), QRectF(10.0, 20.0, 30.0, 40.0));

    state.applyDrawGeometrySyncResult(true);
    expectTextureUpdatePlan(state, KiriView::ImageRenderNodeTextureUpdatePlan::ReuseTextures);
    QVERIFY(!state.setTargetRect(QRectF(10.0, 20.0, 30.0, 40.0)));
    expectTextureUpdatePlan(state, KiriView::ImageRenderNodeTextureUpdatePlan::ReuseTextures);
}

void TestImageRenderNodeState::drawGeometrySyncFailurePromotesTextureRebuild()
{
    KiriView::ImageRenderNodeState state;
    state.setSurface(false, 7);
    state.markTexturesUploaded();
    state.setTargetRect(QRectF(10.0, 20.0, 30.0, 40.0));

    state.applyDrawGeometrySyncResult(false);

    expectTextureUpdatePlan(state, KiriView::ImageRenderNodeTextureUpdatePlan::RebuildTextures);
}

void TestImageRenderNodeState::rotationIsNormalizedBeforeComparison()
{
    KiriView::ImageRenderNodeState state;
    state.setSurface(false, 1);
    state.markTexturesUploaded();

    QVERIFY(state.setRotationDegrees(450));
    QCOMPARE(state.rotationDegrees(), 90);
    expectTextureUpdatePlan(
        state, KiriView::ImageRenderNodeTextureUpdatePlan::SynchronizeDrawGeometry);
    state.applyDrawGeometrySyncResult(true);

    QVERIFY(!state.setRotationDegrees(90));
    expectTextureUpdatePlan(state, KiriView::ImageRenderNodeTextureUpdatePlan::ReuseTextures);

    QVERIFY(state.setRotationDegrees(-90));
    QCOMPARE(state.rotationDegrees(), 270);
    expectTextureUpdatePlan(
        state, KiriView::ImageRenderNodeTextureUpdatePlan::SynchronizeDrawGeometry);
}

void TestImageRenderNodeState::resetUploadedResourcesKeepsPublicInputs()
{
    KiriView::ImageRenderNodeState state;
    state.setSurface(false, 3);
    QVERIFY(state.setTargetRect(QRectF(1.0, 2.0, 3.0, 4.0)));
    QVERIFY(state.setRotationDegrees(180));
    state.markTexturesUploaded();

    state.resetUploadedResources();

    QCOMPARE(state.targetRect(), QRectF(1.0, 2.0, 3.0, 4.0));
    QCOMPARE(state.rotationDegrees(), 180);
    expectTextureUpdatePlan(state, KiriView::ImageRenderNodeTextureUpdatePlan::RebuildTextures);
}

QTEST_GUILESS_MAIN(TestImageRenderNodeState)

#include "test_imagerendernodestate.moc"
