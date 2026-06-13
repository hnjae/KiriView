// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/imagezoomstateconversion.h"

#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <QUrl>

class TestImageZoomStateConversion : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void snapshotRoundTripsPlainZoomValues();
    void loadedZoomRoundTripsPlainZoomValues();
};

void TestImageZoomStateConversion::snapshotRoundTripsPlainZoomValues()
{
    const QUrl containerUrl = QUrl::fromLocalFile(QStringLiteral("/images/"));
    const kiriview::ImageZoomSnapshot snapshot {
        QSize(640, 480),
        QSizeF(320.0, 240.0),
        QSizeF(1280.0, 960.0),
        200.0,
        kiriview::ImageZoomMode::Manual,
        containerUrl,
    };

    const kiriview::RustImageZoomState rustState = kiriview::Bridge::rustImageZoomState(snapshot);
    QCOMPARE(rustState.image_width, 640);
    QCOMPARE(rustState.image_height, 480);
    QCOMPARE(rustState.viewport_width, 320.0);
    QCOMPARE(rustState.viewport_height, 240.0);
    QCOMPARE(rustState.display_width, 1280.0);
    QCOMPARE(rustState.display_height, 960.0);
    QCOMPARE(rustState.zoom_percent, 200.0);
    QVERIFY(rustState.zoom_mode == kiriview::RustImageZoomMode::Manual);

    const kiriview::ImageZoomSnapshot converted
        = kiriview::Bridge::imageZoomSnapshotFromRust(rustState, containerUrl);
    QCOMPARE(converted.imageSize, snapshot.imageSize);
    QCOMPARE(converted.viewportSize, snapshot.viewportSize);
    QCOMPARE(converted.displaySize, snapshot.displaySize);
    QCOMPARE(converted.zoomPercent, snapshot.zoomPercent);
    QVERIFY(converted.zoomMode == kiriview::ImageZoomMode::Manual);
    QCOMPARE(converted.containerUrl, containerUrl);
}

void TestImageZoomStateConversion::loadedZoomRoundTripsPlainZoomValues()
{
    const kiriview::LoadedImageZoom zoom {
        kiriview::ImageZoomMode::FitWidth,
        175.0,
        QSizeF(700.0, 500.0),
    };

    const kiriview::RustLoadedImageZoom rustZoom = kiriview::Bridge::rustLoadedImageZoom(zoom);
    QVERIFY(rustZoom.zoom_mode == kiriview::RustImageZoomMode::FitWidth);
    QCOMPARE(rustZoom.zoom_percent, 175.0);
    QCOMPARE(rustZoom.display_width, 700.0);
    QCOMPARE(rustZoom.display_height, 500.0);

    const kiriview::LoadedImageZoom converted = kiriview::Bridge::loadedImageZoomFromRust(rustZoom);
    QVERIFY(converted.zoomMode == kiriview::ImageZoomMode::FitWidth);
    QCOMPARE(converted.zoomPercent, 175.0);
    QCOMPARE(converted.displaySize, QSizeF(700.0, 500.0));
}

QTEST_GUILESS_MAIN(TestImageZoomStateConversion)

#include "test_imagezoomstateconversion.moc"
