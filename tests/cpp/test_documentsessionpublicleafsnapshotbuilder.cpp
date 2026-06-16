// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionpublicleafsnapshotbuilder.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>

class TestDocumentSessionPublicLeafSnapshotBuilder : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void buildsImageLeafSnapshotFromCommittedImageFacts();
    void buildsVideoLeafSnapshotFromCommittedVideoFacts();
};

void TestDocumentSessionPublicLeafSnapshotBuilder::buildsImageLeafSnapshotFromCommittedImageFacts()
{
    kiriview::DocumentSessionImageDocumentSnapshot leaf;
    leaf.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/book.cbz"));
    leaf.displayedUrl = QUrl::fromLocalFile(QStringLiteral("/media/page.png"));
    leaf.windowTitleFileName = QStringLiteral("page.png");
    leaf.primaryImageSize = QSize(320, 200);
    leaf.ready = true;
    leaf.error = true;
    leaf.unsupportedOpenedCollectionVideo = true;
    leaf.fileDeletionInProgress = true;
    leaf.openedCollectionScopeActive = true;
    leaf.ordinaryDirectMediaScopeActive = true;
    leaf.containerNavigationAvailable = true;
    leaf.twoPageModeEnabled = true;
    leaf.twoPageModeAvailable = true;
    leaf.rightToLeftReadingEnabled = true;
    leaf.rightToLeftReadingAvailable = true;
    leaf.fitModeSelected = true;
    leaf.fitHeightModeSelected = true;
    leaf.fitWidthModeSelected = true;
    leaf.zoomPercentKnown = true;
    leaf.zoomPercent = 125.0;
    leaf.errorString = QStringLiteral("image error");
    leaf.activeNavigationSnapshot.known = true;
    leaf.pageNavigationSnapshot.state.currentIndex = 7;

    const kiriview::DocumentSessionPublicImageLeafSnapshot snapshot
        = kiriview::buildDocumentSessionPublicImageLeafSnapshot(leaf);

    QCOMPARE(snapshot.sourceUrl, leaf.sourceUrl);
    QVERIFY(snapshot.sourceMayRepresentDocument);
    QCOMPARE(snapshot.pageNavigation.known, leaf.activeNavigationSnapshot.known);
    QCOMPARE(snapshot.pageNavigationRows.state.currentIndex, 7);
    QCOMPARE(snapshot.displayedUrl, leaf.displayedUrl);
    QCOMPARE(snapshot.windowTitleFileName, QStringLiteral("page.png"));
    QCOMPARE(snapshot.directMediaSize, QSize(320, 200));
    QVERIFY(snapshot.readyForDeletion);
    QVERIFY(snapshot.readyForInformation);
    QVERIFY(snapshot.error);
    QVERIFY(snapshot.fileDeletionInProgress);
    QVERIFY(snapshot.openedCollectionScopeActive);
    QVERIFY(snapshot.ordinaryDirectMediaScopeActive);
    QVERIFY(snapshot.unsupportedOpenedCollectionVideo);
    QVERIFY(!snapshot.directImageReplacementPending);
    QVERIFY(snapshot.containerNavigationAvailable);
    QVERIFY(snapshot.twoPageModeEnabled);
    QVERIFY(snapshot.twoPageModeAvailable);
    QVERIFY(snapshot.rightToLeftReadingEnabled);
    QVERIFY(snapshot.rightToLeftReadingAvailable);
    QVERIFY(snapshot.fitModeSelected);
    QVERIFY(snapshot.fitHeightModeSelected);
    QVERIFY(snapshot.fitWidthModeSelected);
    QVERIFY(snapshot.zoomPercentKnown);
    QCOMPARE(snapshot.zoomPercent, 125.0);
    QCOMPARE(snapshot.errorString, QStringLiteral("image error"));
}

void TestDocumentSessionPublicLeafSnapshotBuilder::buildsVideoLeafSnapshotFromCommittedVideoFacts()
{
    kiriview::DocumentSessionVideoDocumentSnapshot leaf;
    leaf.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/clip.mp4"));
    leaf.windowTitleFileName = QStringLiteral("clip.mp4");
    leaf.videoSize = QSize(640, 360);
    leaf.ready = true;
    leaf.error = true;
    leaf.hasVideo = true;
    leaf.zoomPercentKnown = true;
    leaf.zoomPercent = 75;
    leaf.errorString = QStringLiteral("video error");

    const kiriview::DocumentSessionPublicVideoLeafSnapshot snapshot
        = kiriview::buildDocumentSessionPublicVideoLeafSnapshot(leaf);

    QCOMPARE(snapshot.sourceUrl, leaf.sourceUrl);
    QCOMPARE(snapshot.windowTitleFileName, QStringLiteral("clip.mp4"));
    QCOMPARE(snapshot.directMediaSize, QSize(640, 360));
    QVERIFY(snapshot.ready);
    QVERIFY(snapshot.error);
    QVERIFY(snapshot.hasVideo);
    QVERIFY(snapshot.sourcePresent);
    QVERIFY(snapshot.zoomPercentKnown);
    QCOMPARE(snapshot.zoomPercent, 75);
    QCOMPARE(snapshot.errorString, QStringLiteral("video error"));
}

QTEST_GUILESS_MAIN(TestDocumentSessionPublicLeafSnapshotBuilder)

#include "test_documentsessionpublicleafsnapshotbuilder.moc"
