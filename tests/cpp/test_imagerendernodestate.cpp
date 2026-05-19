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
    void surfaceRevisionInvalidatesUploadedTextures();
    void surfacePayloadChangeInvalidatesCurrentRevision();
    void targetGeometryOnlyInvalidatesDrawGeometry();
    void rotationIsNormalizedBeforeComparison();
    void resetUploadedResourcesKeepsPublicInputs();
};

void TestImageRenderNodeState::surfaceRevisionInvalidatesUploadedTextures()
{
    KiriView::ImageRenderNodeState state;

    QVERIFY(state.setSurfaceRevision(10));
    state.markTexturesUploaded();
    QVERIFY(state.uploadedTexturesAreCurrent());
    QVERIFY(!state.drawGeometryDirty());

    QVERIFY(!state.setSurfaceRevision(10));
    QVERIFY(state.uploadedTexturesAreCurrent());

    QVERIFY(state.setSurfaceRevision(11));
    QVERIFY(!state.uploadedTexturesAreCurrent());
    QVERIFY(state.drawGeometryDirty());
}

void TestImageRenderNodeState::surfacePayloadChangeInvalidatesCurrentRevision()
{
    KiriView::ImageRenderNodeState state;
    QVERIFY(state.setSurfaceRevision(10));
    state.markTexturesUploaded();
    QVERIFY(state.uploadedTexturesAreCurrent());

    state.markSurfaceChanged();

    QVERIFY(!state.uploadedTexturesAreCurrent());
    QVERIFY(state.drawGeometryDirty());
}

void TestImageRenderNodeState::targetGeometryOnlyInvalidatesDrawGeometry()
{
    KiriView::ImageRenderNodeState state;
    QVERIFY(state.setSurfaceRevision(7));
    state.markTexturesUploaded();

    QVERIFY(state.setTargetRect(QRectF(10.0, 20.0, 30.0, 40.0)));
    QVERIFY(state.uploadedTexturesAreCurrent());
    QVERIFY(state.drawGeometryDirty());
    QCOMPARE(state.targetRect(), QRectF(10.0, 20.0, 30.0, 40.0));

    state.markDrawGeometrySynchronized();
    QVERIFY(!state.setTargetRect(QRectF(10.0, 20.0, 30.0, 40.0)));
    QVERIFY(!state.drawGeometryDirty());
}

void TestImageRenderNodeState::rotationIsNormalizedBeforeComparison()
{
    KiriView::ImageRenderNodeState state;

    QVERIFY(state.setRotationDegrees(450));
    QCOMPARE(state.rotationDegrees(), 90);
    state.markDrawGeometrySynchronized();

    QVERIFY(!state.setRotationDegrees(90));
    QVERIFY(!state.drawGeometryDirty());

    QVERIFY(state.setRotationDegrees(-90));
    QCOMPARE(state.rotationDegrees(), 270);
    QVERIFY(state.drawGeometryDirty());
}

void TestImageRenderNodeState::resetUploadedResourcesKeepsPublicInputs()
{
    KiriView::ImageRenderNodeState state;
    QVERIFY(state.setSurfaceRevision(3));
    QVERIFY(state.setTargetRect(QRectF(1.0, 2.0, 3.0, 4.0)));
    QVERIFY(state.setRotationDegrees(180));
    state.markTexturesUploaded();

    state.resetUploadedResources();

    QCOMPARE(state.targetRect(), QRectF(1.0, 2.0, 3.0, 4.0));
    QCOMPARE(state.rotationDegrees(), 180);
    QVERIFY(!state.uploadedTexturesAreCurrent());
    QVERIFY(state.drawGeometryDirty());
}

QTEST_GUILESS_MAIN(TestImageRenderNodeState)

#include "test_imagerendernodestate.moc"
