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
    void openedCollectionVideoProjectsImagePageNavigationAndCollectionTitle();
    void openedCollectionVideoPublicSnapshotKeepsCollectionToolbarScope();
    void deletionMasksNavigationDispatchWithoutDroppingTitleCounter();
    void imageDeletionAvailabilityRequiresReadyImageWithoutPendingReplacement();
    void videoDeletionAvailabilityRequiresSourceWithoutError();
    void deletionProgressSuppressesDeletionAvailability();
    void publicSnapshotProjectsRevisionedMixedMediaState();
};

void TestDocumentSessionPublicProjection::emptySessionProjectsUnavailableNavigationAndEmptyTitle()
{
    const kiriview::DocumentSessionPublicProjection projection
        = kiriview::projectDocumentSessionPublicState({});

    QCOMPARE(projection.sourceKind, kiriview::ActiveNavigationSourceKind::None);
    QCOMPARE(projection.boundaryScope, kiriview::ActiveNavigationBoundaryScope::None);
    QVERIFY(!projection.activeNavigation.available);
    QCOMPARE(projection.windowTitleSubject, QString());
    QVERIFY(!projection.displayedFileDeletionAvailable);
}

void TestDocumentSessionPublicProjection::
    directImageProjectsDirectMediaNavigationAndIntrinsicSizeTitle()
{
    const kiriview::DocumentSessionPublicProjection projection
        = kiriview::projectDocumentSessionPublicState(
            kiriview::DocumentSessionPublicProjectionInput {
                kiriview::DocumentSessionKind::Image,
                true,
                false,
                false,
                kiriview::DirectMediaActiveNavigationInput {
                    kiriview::DirectMediaNavigationBoundaryState { false, true, true, false, 1, 3 },
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

    QCOMPARE(projection.sourceKind, kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia);
    QCOMPARE(projection.boundaryScope, kiriview::ActiveNavigationBoundaryScope::DirectMedia);
    QVERIFY(projection.activeNavigation.known);
    QVERIFY(projection.activeNavigation.canOpenNext);
    QCOMPARE(projection.activeNavigation.currentNumber, 1);
    QCOMPARE(projection.activeNavigation.count, 3);
    QCOMPARE(projection.windowTitleSubject, QStringLiteral("01.png – 640×480"));
    QVERIFY(projection.displayedFileDeletionAvailable);
}

void TestDocumentSessionPublicProjection::archiveImageProjectsPageNavigationAndCounterTitle()
{
    const kiriview::DocumentSessionPublicProjection projection
        = kiriview::projectDocumentSessionPublicState(
            kiriview::DocumentSessionPublicProjectionInput {
                kiriview::DocumentSessionKind::Image,
                false,
                true,
                false,
                {},
                kiriview::ImageDocumentPageActiveNavigationSnapshot {
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

    QCOMPARE(projection.sourceKind, kiriview::ActiveNavigationSourceKind::ImageDocumentPages);
    QCOMPARE(projection.boundaryScope, kiriview::ActiveNavigationBoundaryScope::ImageDocumentPage);
    QVERIFY(projection.activeNavigation.known);
    QVERIFY(projection.activeNavigation.canOpenPrevious);
    QVERIFY(projection.activeNavigation.canOpenNext);
    QCOMPARE(projection.activeNavigation.currentNumber, 2);
    QCOMPARE(projection.activeNavigation.count, 5);
    QCOMPARE(projection.windowTitleSubject, QStringLiteral("book.cbz – 2/5"));
    QVERIFY(projection.displayedFileDeletionAvailable);
}

void TestDocumentSessionPublicProjection::
    openedCollectionVideoProjectsImagePageNavigationAndCollectionTitle()
{
    kiriview::DocumentSessionPublicProjectionInput input;
    input.documentKind = kiriview::DocumentSessionKind::Video;
    input.openedCollectionVideoActive = true;
    input.imageDocumentPageNavigation = kiriview::ImageDocumentPageActiveNavigationSnapshot {
        true,
        true,
        false,
        false,
        true,
        3,
        5,
    };
    input.imageWindowTitleFileName = QStringLiteral("book.cbz");
    input.videoWindowTitleFileName = QStringLiteral("clip.mp4");
    input.videoDirectMediaSize = QSize(1920, 1080);
    input.videoSourcePresent = true;

    const kiriview::DocumentSessionPublicProjection projection
        = kiriview::projectDocumentSessionPublicState(input);

    QCOMPARE(projection.sourceKind, kiriview::ActiveNavigationSourceKind::ImageDocumentPages);
    QCOMPARE(projection.boundaryScope, kiriview::ActiveNavigationBoundaryScope::ImageDocumentPage);
    QVERIFY(projection.activeNavigation.known);
    QVERIFY(projection.activeNavigation.canOpenPrevious);
    QVERIFY(!projection.activeNavigation.canOpenNext);
    QCOMPARE(projection.activeNavigation.currentNumber, 3);
    QCOMPARE(projection.activeNavigation.count, 5);
    QCOMPARE(projection.windowTitleSubject, QStringLiteral("book.cbz – 3/5"));
    QVERIFY(projection.displayedFileDeletionAvailable);
}

void TestDocumentSessionPublicProjection::
    openedCollectionVideoPublicSnapshotKeepsCollectionToolbarScope()
{
    kiriview::DocumentSessionPublicSnapshotInput input;
    input.inputRevision = 8;
    input.session.sourceUrl = QUrl(QStringLiteral("zip:///books/book.cbz!/02.mp4"));
    input.session.documentKind = kiriview::DocumentSessionKind::Video;
    input.session.openedCollectionVideoActive = true;
    input.image.sourceUrl = input.session.sourceUrl;
    input.image.sourceKind = kiriview::ImageDocumentPageKind::Video;
    input.image.openedCollectionScopeActive = true;
    input.image.windowTitleFileName = QStringLiteral("book.cbz");
    input.image.pageNavigation = kiriview::ImageDocumentPageActiveNavigationSnapshot {
        true,
        true,
        true,
        false,
        false,
        2,
        4,
    };
    input.image.twoPageModeEnabled = true;
    input.image.twoPageModeAvailable = true;
    input.image.rightToLeftReadingEnabled = true;
    input.image.rightToLeftReadingAvailable = true;
    input.image.fitModeSelected = true;
    input.video.sourceUrl = input.session.sourceUrl;
    input.video.windowTitleFileName = QStringLiteral("02.mp4");
    input.video.sourcePresent = true;
    input.video.ready = true;
    input.video.hasVideo = true;
    input.video.zoomPercentKnown = true;
    input.video.zoomPercent = 71;

    const kiriview::DocumentSessionPublicSnapshot snapshot
        = kiriview::projectDocumentSessionPublicSnapshot(input, 4);

    QCOMPARE(snapshot.documentKind, kiriview::DocumentSessionKind::Video);
    QVERIFY(snapshot.activeImageOpenedCollectionScopeActive);
    QVERIFY(!snapshot.activeImageReady);
    QVERIFY(snapshot.activeVideoReady);
    QVERIFY(snapshot.actionAvailability.twoPageModeActive);
    QVERIFY(snapshot.actionAvailability.twoPageModeAvailable);
    QVERIFY(snapshot.actionAvailability.rightToLeftReadingActive);
    QVERIFY(snapshot.actionAvailability.rightToLeftReadingAvailable);
    QVERIFY(snapshot.actionAvailability.fitModeSelected);
    QVERIFY(!snapshot.actionAvailability.imageReady);
    QVERIFY(snapshot.activeZoom.available);
    QVERIFY(snapshot.activeZoom.known);
    QCOMPARE(snapshot.activeZoom.percent, 71.0);
    QVERIFY(!snapshot.activeZoom.editable);
    QCOMPARE(
        snapshot.projection.sourceKind, kiriview::ActiveNavigationSourceKind::ImageDocumentPages);
    QCOMPARE(snapshot.projection.activeNavigation.currentNumber, 2);
    QCOMPARE(snapshot.projection.activeNavigation.count, 4);
    QCOMPARE(snapshot.projection.windowTitleSubject, QStringLiteral("book.cbz – 2/4"));
}

void TestDocumentSessionPublicProjection::
    deletionMasksNavigationDispatchWithoutDroppingTitleCounter()
{
    const kiriview::DocumentSessionPublicProjection projection
        = kiriview::projectDocumentSessionPublicState(
            kiriview::DocumentSessionPublicProjectionInput {
                kiriview::DocumentSessionKind::Video,
                false,
                false,
                true,
                kiriview::DirectMediaActiveNavigationInput {
                    kiriview::DirectMediaNavigationBoundaryState { true, true, false, false, 2, 4 },
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

    QCOMPARE(projection.sourceKind, kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia);
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
    kiriview::DocumentSessionPublicProjectionInput input;
    input.documentKind = kiriview::DocumentSessionKind::Image;
    input.imageReadyForDeletion = true;

    kiriview::DocumentSessionPublicProjection projection
        = kiriview::projectDocumentSessionPublicState(input);
    QVERIFY(projection.displayedFileDeletionAvailable);

    input.imageReadyForDeletion = false;
    projection = kiriview::projectDocumentSessionPublicState(input);
    QVERIFY(!projection.displayedFileDeletionAvailable);

    input.directImageLoadMayUseImageDocumentSourceScope = true;
    input.imageReadyForDeletion = true;
    input.directImageReplacementPending = true;
    projection = kiriview::projectDocumentSessionPublicState(input);
    QVERIFY(!projection.displayedFileDeletionAvailable);
}

void TestDocumentSessionPublicProjection::videoDeletionAvailabilityRequiresSourceWithoutError()
{
    kiriview::DocumentSessionPublicProjectionInput input;
    input.documentKind = kiriview::DocumentSessionKind::Video;
    input.videoSourcePresent = true;

    kiriview::DocumentSessionPublicProjection projection
        = kiriview::projectDocumentSessionPublicState(input);
    QVERIFY(projection.displayedFileDeletionAvailable);

    input.videoSourcePresent = false;
    projection = kiriview::projectDocumentSessionPublicState(input);
    QVERIFY(!projection.displayedFileDeletionAvailable);

    input.videoSourcePresent = true;
    input.videoError = true;
    projection = kiriview::projectDocumentSessionPublicState(input);
    QVERIFY(!projection.displayedFileDeletionAvailable);
}

void TestDocumentSessionPublicProjection::deletionProgressSuppressesDeletionAvailability()
{
    kiriview::DocumentSessionPublicProjectionInput input;
    input.documentKind = kiriview::DocumentSessionKind::Image;
    input.imageReadyForDeletion = true;
    input.fileDeletionInProgress = true;

    kiriview::DocumentSessionPublicProjection projection
        = kiriview::projectDocumentSessionPublicState(input);
    QVERIFY(!projection.displayedFileDeletionAvailable);
}

void TestDocumentSessionPublicProjection::publicSnapshotProjectsRevisionedMixedMediaState()
{
    kiriview::DocumentSessionPublicSnapshotInput input;
    input.inputRevision = 7;
    input.session.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/clip.mp4"));
    input.session.documentKind = kiriview::DocumentSessionKind::Video;
    input.session.directMediaNavigation = kiriview::DirectMediaActiveNavigationInput {
        kiriview::DirectMediaNavigationBoundaryState { true, false, false, true, 2, 2 },
        true,
    };
    input.video.windowTitleFileName = QStringLiteral("clip.mp4");
    input.video.directMediaSize = QSize(1920, 1080);
    input.video.sourcePresent = true;
    input.video.zoomPercentKnown = true;
    input.video.zoomPercent = 75;
    input.video.errorString = QStringLiteral("decode details");
    input.operations.displayedMediaOpenWithAvailable = true;

    const kiriview::DocumentSessionPublicSnapshot snapshot
        = kiriview::projectDocumentSessionPublicSnapshot(input, 3);

    QCOMPARE(snapshot.revision, quint64(3));
    QCOMPARE(snapshot.inputRevision, quint64(7));
    QCOMPARE(snapshot.sourceUrl, input.session.sourceUrl);
    QCOMPARE(snapshot.documentKind, kiriview::DocumentSessionKind::Video);
    QCOMPARE(snapshot.errorString, QStringLiteral("decode details"));
    QVERIFY(snapshot.activeZoom.available);
    QVERIFY(snapshot.activeZoom.known);
    QCOMPARE(snapshot.activeZoom.percent, 75.0);
    QVERIFY(!snapshot.activeZoom.editable);
    QCOMPARE(
        snapshot.projection.sourceKind, kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia);
    QCOMPARE(snapshot.projection.activeNavigation.currentNumber, 2);
    QCOMPARE(snapshot.projection.activeNavigation.count, 2);
    QCOMPARE(snapshot.projection.windowTitleSubject, QStringLiteral("clip.mp4 – 1920×1080"));
    QVERIFY(snapshot.projection.displayedFileDeletionAvailable);
    QVERIFY(snapshot.projection.displayedMediaOpenWithAvailable);
}

QTEST_GUILESS_MAIN(TestDocumentSessionPublicProjection)

#include "test_documentsessionpublicprojection.moc"
