// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageviewportinteraction.h"

#include <QObject>
#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QTest>
#include <cmath>

namespace {
kiriview::ImageViewportInteractionSnapshot interactionSnapshot(bool rightToLeftReading = false)
{
    return kiriview::ImageViewportInteractionSnapshot {
        QSize(400, 400),
        QSizeF(100.0, 100.0),
        QSizeF(400.0, 300.0),
        1.0,
        rightToLeftReading,
    };
}

bool approximatelyEqual(qreal left, qreal right) { return std::abs(left - right) < 0.001; }

void comparePoint(const QPointF& actual, const QPointF& expected)
{
    QVERIFY2(approximatelyEqual(actual.x(), expected.x()),
        qPrintable(QStringLiteral("x=%1").arg(actual.x())));
    QVERIFY2(approximatelyEqual(actual.y(), expected.y()),
        qPrintable(QStringLiteral("y=%1").arg(actual.y())));
}
}

class TestImageViewportInteraction : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void delegatesViewportGeometryFromSnapshot();
    void scanStartStateOwnsDisplayedImageInitialPosition();
    void rightToLeftSnapshotChangesScanDirection();
    void anchoredZoomUsesSnapshotImageSize();
    void nearestImageViewportPointUsesSnapshotGeometry();
};

void TestImageViewportInteraction::delegatesViewportGeometryFromSnapshot()
{
    kiriview::ImageViewportInteraction interaction;
    const kiriview::ImageViewportInteractionSnapshot snapshot = interactionSnapshot();

    comparePoint(interaction.panContentPosition(snapshot, QPointF(180.0, 0.0), QPointF(80.0, 0.0)),
        QPointF(260.0, 0.0));
    comparePoint(
        interaction.nextScanContentPosition(snapshot, QPointF(225.0, 0.0)), QPointF(262.5, 0.0));
    QVERIFY(interaction.viewportPointInsideImage(snapshot, QPointF(0.0, 0.0), QPointF(50.0, 50.0)));
    QVERIFY(
        !interaction.viewportPointInsideImage(snapshot, QPointF(0.0, 0.0), QPointF(50.0, 320.0)));
}

void TestImageViewportInteraction::scanStartStateOwnsDisplayedImageInitialPosition()
{
    kiriview::ImageViewportInteraction interaction;
    const kiriview::ImageViewportInteractionSnapshot snapshot = interactionSnapshot();

    comparePoint(interaction.displayedImageInitialContentPosition(snapshot), QPointF(0.0, 0.0));

    interaction.requestNextDisplayedImageFinalScanStart();
    QVERIFY(interaction.pendingFinalScanStart());
    QCOMPARE(interaction.beginDisplayedImage(), kiriview::ImageViewportScanStart::Final);
    QVERIFY(!interaction.pendingFinalScanStart());
    comparePoint(interaction.displayedImageInitialContentPosition(snapshot), QPointF(300.0, 200.0));

    QCOMPARE(interaction.beginDisplayedImage(), kiriview::ImageViewportScanStart::Initial);
    comparePoint(interaction.displayedImageInitialContentPosition(snapshot), QPointF(0.0, 0.0));
}

void TestImageViewportInteraction::rightToLeftSnapshotChangesScanDirection()
{
    kiriview::ImageViewportInteraction interaction;
    const kiriview::ImageViewportInteractionSnapshot snapshot = interactionSnapshot(true);

    comparePoint(interaction.initialScanContentPosition(snapshot), QPointF(300.0, 0.0));
    comparePoint(interaction.finalScanContentPosition(snapshot), QPointF(0.0, 200.0));
    comparePoint(
        interaction.nextScanContentPosition(snapshot, QPointF(300.0, 0.0)), QPointF(262.5, 0.0));
}

void TestImageViewportInteraction::anchoredZoomUsesSnapshotImageSize()
{
    kiriview::ImageViewportInteraction interaction;
    kiriview::ImageViewportInteractionSnapshot snapshot = interactionSnapshot();
    snapshot.imageSize = QSize(100, 100);
    snapshot.displaySize = QSizeF(100.0, 100.0);

    comparePoint(
        interaction.zoomContentPosition(snapshot, QPointF(0.0, 0.0), QPointF(25.0, 25.0), 200.0),
        QPointF(25.0, 25.0));
}

void TestImageViewportInteraction::nearestImageViewportPointUsesSnapshotGeometry()
{
    kiriview::ImageViewportInteraction interaction;
    const kiriview::ImageViewportInteractionSnapshot snapshot = interactionSnapshot();

    comparePoint(
        interaction.nearestImageViewportPoint(snapshot, QPointF(0.0, 0.0), QPointF(50.0, 320.0)),
        QPointF(50.0, 300.0));
}

QTEST_GUILESS_MAIN(TestImageViewportInteraction)

#include "test_imageviewportinteraction.moc"
