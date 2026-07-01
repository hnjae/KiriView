// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionactivezoom.h"
#include "session/documentsessionpublicprojection.h"

#include <QObject>
#include <QTest>

class TestDocumentSessionActiveZoom : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void derivesImageVideoAndEmptyZoomSnapshots();
    void pendingImageNavigationDoesNotExposeStaleImageZoom();
};

void TestDocumentSessionActiveZoom::derivesImageVideoAndEmptyZoomSnapshots()
{
    kiriview::DocumentSessionPublicImageLeafSnapshot image;
    image.readyForInformation = true;
    image.zoomPercentKnown = true;
    image.zoomPercent = 125.0;
    kiriview::DocumentSessionPublicVideoLeafSnapshot video;

    kiriview::ActiveZoomSnapshot snapshot = kiriview::documentSessionActiveZoomSnapshot(
        kiriview::DocumentSessionKind::Image, image, video);
    QVERIFY(snapshot.available);
    QVERIFY(snapshot.known);
    QCOMPARE(snapshot.percent, 125.0);
    QVERIFY(snapshot.editable);

    image.zoomPercentKnown = false;
    snapshot = kiriview::documentSessionActiveZoomSnapshot(
        kiriview::DocumentSessionKind::Image, image, video);
    QVERIFY(!snapshot.available);
    QVERIFY(!snapshot.known);
    QCOMPARE(snapshot.percent, 0.0);
    QVERIFY(!snapshot.editable);

    video.zoomPercentKnown = true;
    video.zoomPercent = 67;
    snapshot = kiriview::documentSessionActiveZoomSnapshot(
        kiriview::DocumentSessionKind::Video, image, video);
    QVERIFY(snapshot.available);
    QVERIFY(snapshot.known);
    QCOMPARE(snapshot.percent, 67.0);
    QVERIFY(!snapshot.editable);

    video.zoomPercentKnown = false;
    snapshot = kiriview::documentSessionActiveZoomSnapshot(
        kiriview::DocumentSessionKind::Video, image, video);
    QVERIFY(snapshot.available);
    QVERIFY(!snapshot.known);
    QCOMPARE(snapshot.percent, 0.0);
    QVERIFY(!snapshot.editable);

    snapshot = kiriview::documentSessionActiveZoomSnapshot(
        kiriview::DocumentSessionKind::Empty, image, video);
    QVERIFY(!snapshot.available);
    QVERIFY(!snapshot.known);
    QCOMPARE(snapshot.percent, 0.0);
    QVERIFY(!snapshot.editable);
}

void TestDocumentSessionActiveZoom::pendingImageNavigationDoesNotExposeStaleImageZoom()
{
    kiriview::DocumentSessionPublicImageLeafSnapshot image;
    image.zoomPercentKnown = true;
    image.zoomPercent = 125.0;
    kiriview::DocumentSessionPublicVideoLeafSnapshot video;

    const kiriview::ActiveZoomSnapshot snapshot = kiriview::documentSessionActiveZoomSnapshot(
        kiriview::DocumentSessionKind::Image, image, video);

    QVERIFY(!snapshot.available);
    QVERIFY(!snapshot.known);
    QCOMPARE(snapshot.percent, 0.0);
    QVERIFY(!snapshot.editable);
}

QTEST_GUILESS_MAIN(TestDocumentSessionActiveZoom)

#include "test_documentsessionactivezoom.moc"
