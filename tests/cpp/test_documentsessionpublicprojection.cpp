// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionpublicprojection.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>

class TestDocumentSessionPublicProjection : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptySessionProjectsUnavailableNavigationAndEmptyTitle();
    void directImageProjectsDirectMediaNavigationAndIntrinsicSizeTitle();
    void archiveImageProjectsPageNavigationAndCounterTitle();
    void deletionMasksNavigationDispatchWithoutDroppingTitleCounter();
    void imageDeletionAvailabilityRequiresReadyImageWithoutPendingReplacement();
    void videoDeletionAvailabilityRequiresSourceWithoutError();
    void deletionProgressSuppressesDeletionAvailability();
    void publicSnapshotProjectsRevisionedMixedMediaState();
};

void TestDocumentSessionPublicProjection::emptySessionProjectsUnavailableNavigationAndEmptyTitle()
{
    const KiriView::DocumentSessionPublicProjection projection
        = KiriView::projectDocumentSessionPublicState({});

    QCOMPARE(projection.sourceKind, KiriView::ActiveNavigationSourceKind::None);
    QCOMPARE(projection.boundaryScope, KiriView::ActiveNavigationBoundaryScope::None);
    QVERIFY(!projection.activeNavigation.available);
    QCOMPARE(projection.windowTitleSubject, QString());
    QVERIFY(!projection.displayedFileDeletionAvailable);
}

void TestDocumentSessionPublicProjection::
    directImageProjectsDirectMediaNavigationAndIntrinsicSizeTitle()
{
    const KiriView::DocumentSessionPublicProjection projection
        = KiriView::projectDocumentSessionPublicState(
            KiriView::DocumentSessionPublicProjectionInput {
                KiriView::DocumentSessionKind::Image,
                true,
                false,
                false,
                KiriView::DirectMediaActiveNavigationInput {
                    KiriView::DirectMediaNavigationBoundaryState { false, true, true, false, 1, 3 },
                    true,
                },
                {},
                QStringLiteral("01.png"),
                QSize(640, 480),
                {},
                {},
                true,
                false,
                false,
                false,
            });

    QCOMPARE(projection.sourceKind, KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia);
    QCOMPARE(projection.boundaryScope, KiriView::ActiveNavigationBoundaryScope::DirectMedia);
    QVERIFY(projection.activeNavigation.known);
    QVERIFY(projection.activeNavigation.canOpenNext);
    QCOMPARE(projection.activeNavigation.currentNumber, 1);
    QCOMPARE(projection.activeNavigation.count, 3);
    QCOMPARE(projection.windowTitleSubject, QStringLiteral("01.png – 640×480"));
    QVERIFY(projection.displayedFileDeletionAvailable);
}

void TestDocumentSessionPublicProjection::archiveImageProjectsPageNavigationAndCounterTitle()
{
    const KiriView::DocumentSessionPublicProjection projection
        = KiriView::projectDocumentSessionPublicState(
            KiriView::DocumentSessionPublicProjectionInput {
                KiriView::DocumentSessionKind::Image,
                false,
                true,
                false,
                {},
                KiriView::ImageDocumentPageActiveNavigationSnapshot {
                    true, true, true, false, false, 2, 5 },
                QStringLiteral("book.cbz"),
                QSize(640, 480),
                {},
                {},
                true,
                false,
                false,
                false,
            });

    QCOMPARE(projection.sourceKind, KiriView::ActiveNavigationSourceKind::ImageDocumentPages);
    QCOMPARE(projection.boundaryScope, KiriView::ActiveNavigationBoundaryScope::ImageDocumentPage);
    QVERIFY(projection.activeNavigation.known);
    QVERIFY(projection.activeNavigation.canOpenPrevious);
    QVERIFY(projection.activeNavigation.canOpenNext);
    QCOMPARE(projection.activeNavigation.currentNumber, 2);
    QCOMPARE(projection.activeNavigation.count, 5);
    QCOMPARE(projection.windowTitleSubject, QStringLiteral("book.cbz – 2/5"));
    QVERIFY(projection.displayedFileDeletionAvailable);
}

void TestDocumentSessionPublicProjection::
    deletionMasksNavigationDispatchWithoutDroppingTitleCounter()
{
    const KiriView::DocumentSessionPublicProjection projection
        = KiriView::projectDocumentSessionPublicState(
            KiriView::DocumentSessionPublicProjectionInput {
                KiriView::DocumentSessionKind::Video,
                false,
                false,
                true,
                KiriView::DirectMediaActiveNavigationInput {
                    KiriView::DirectMediaNavigationBoundaryState { true, true, false, false, 2, 4 },
                    true,
                },
                {},
                {},
                {},
                QStringLiteral("clip.mp4"),
                QSize(1920, 1080),
                false,
                false,
                true,
                false,
            });

    QCOMPARE(projection.sourceKind, KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia);
    QVERIFY(projection.activeNavigation.known);
    QVERIFY(!projection.activeNavigation.editable);
    QVERIFY(!projection.activeNavigation.canOpenPrevious);
    QVERIFY(!projection.activeNavigation.canOpenNext);
    QCOMPARE(projection.activeNavigation.currentNumber, 2);
    QCOMPARE(projection.activeNavigation.count, 4);
    QCOMPARE(projection.windowTitleSubject, QStringLiteral("clip.mp4 – 1920×1080"));
    QVERIFY(!projection.displayedFileDeletionAvailable);
}

void TestDocumentSessionPublicProjection::
    imageDeletionAvailabilityRequiresReadyImageWithoutPendingReplacement()
{
    KiriView::DocumentSessionPublicProjectionInput input;
    input.documentKind = KiriView::DocumentSessionKind::Image;
    input.imageReadyForDeletion = true;

    KiriView::DocumentSessionPublicProjection projection
        = KiriView::projectDocumentSessionPublicState(input);
    QVERIFY(projection.displayedFileDeletionAvailable);

    input.imageReadyForDeletion = false;
    projection = KiriView::projectDocumentSessionPublicState(input);
    QVERIFY(!projection.displayedFileDeletionAvailable);

    input.directImageLoadMayUseImageDocumentSourceScope = true;
    input.imageReadyForDeletion = true;
    input.directImageReplacementPending = true;
    projection = KiriView::projectDocumentSessionPublicState(input);
    QVERIFY(!projection.displayedFileDeletionAvailable);
}

void TestDocumentSessionPublicProjection::videoDeletionAvailabilityRequiresSourceWithoutError()
{
    KiriView::DocumentSessionPublicProjectionInput input;
    input.documentKind = KiriView::DocumentSessionKind::Video;
    input.videoSourcePresent = true;

    KiriView::DocumentSessionPublicProjection projection
        = KiriView::projectDocumentSessionPublicState(input);
    QVERIFY(projection.displayedFileDeletionAvailable);

    input.videoSourcePresent = false;
    projection = KiriView::projectDocumentSessionPublicState(input);
    QVERIFY(!projection.displayedFileDeletionAvailable);

    input.videoSourcePresent = true;
    input.videoError = true;
    projection = KiriView::projectDocumentSessionPublicState(input);
    QVERIFY(!projection.displayedFileDeletionAvailable);
}

void TestDocumentSessionPublicProjection::deletionProgressSuppressesDeletionAvailability()
{
    KiriView::DocumentSessionPublicProjectionInput input;
    input.documentKind = KiriView::DocumentSessionKind::Image;
    input.imageReadyForDeletion = true;
    input.fileDeletionInProgress = true;

    KiriView::DocumentSessionPublicProjection projection
        = KiriView::projectDocumentSessionPublicState(input);
    QVERIFY(!projection.displayedFileDeletionAvailable);
}

void TestDocumentSessionPublicProjection::publicSnapshotProjectsRevisionedMixedMediaState()
{
    KiriView::DocumentSessionPublicSnapshotInput input;
    input.inputRevision = 7;
    input.session.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/clip.mp4"));
    input.session.documentKind = KiriView::DocumentSessionKind::Video;
    input.session.directMediaNavigation = KiriView::DirectMediaActiveNavigationInput {
        KiriView::DirectMediaNavigationBoundaryState { true, false, false, true, 2, 2 },
        true,
    };
    input.video.windowTitleFileName = QStringLiteral("clip.mp4");
    input.video.directMediaSize = QSize(1920, 1080);
    input.video.sourcePresent = true;
    input.video.zoomPercentKnown = true;
    input.video.zoomPercent = 75;
    input.video.errorString = QStringLiteral("decode details");
    input.operations.displayedMediaOpenWithAvailable = true;

    const KiriView::DocumentSessionPublicSnapshot snapshot
        = KiriView::projectDocumentSessionPublicSnapshot(input, 3);

    QCOMPARE(snapshot.revision, quint64(3));
    QCOMPARE(snapshot.inputRevision, quint64(7));
    QCOMPARE(snapshot.sourceUrl, input.session.sourceUrl);
    QCOMPARE(snapshot.documentKind, KiriView::DocumentSessionKind::Video);
    QCOMPARE(snapshot.errorString, QStringLiteral("decode details"));
    QVERIFY(snapshot.activeZoom.available);
    QVERIFY(snapshot.activeZoom.known);
    QCOMPARE(snapshot.activeZoom.percent, 75.0);
    QVERIFY(!snapshot.activeZoom.editable);
    QCOMPARE(
        snapshot.projection.sourceKind, KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia);
    QCOMPARE(snapshot.projection.activeNavigation.currentNumber, 2);
    QCOMPARE(snapshot.projection.activeNavigation.count, 2);
    QCOMPARE(snapshot.projection.windowTitleSubject, QStringLiteral("clip.mp4 – 1920×1080"));
    QVERIFY(snapshot.projection.displayedFileDeletionAvailable);
    QVERIFY(snapshot.projection.displayedMediaOpenWithAvailable);
}

QTEST_GUILESS_MAIN(TestDocumentSessionPublicProjection)

#include "test_documentsessionpublicprojection.moc"
