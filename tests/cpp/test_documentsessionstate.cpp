// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionstate.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>
#include <vector>

namespace {
KiriView::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl &url)
{
    return KiriView::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}
}

class TestDocumentSessionState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sourceIdentityUpdatesInternalStateUntilSnapshotCommit();
    void documentKindUpdatesInternalStateUntilSnapshotCommit();
    void activeZoomReadoutPublishesThroughSnapshotCommit();
    void fileDeletionProgressPublishesThroughSnapshotCommit();
    void directMediaNavigationSnapshotOwnsBoundaryAndCandidates();
    void publicProjectionCommitsValuesBeforePublishing();
    void publicProjectionOnlyNotifiesChangedOutputs();
    void publicSnapshotCommitsOneRevisionedBatch();
    void unchangedPublicSnapshotDoesNotAdvanceRevision();
    void publishDeduplicatesChangesInOrder();
};

void TestDocumentSessionState::sourceIdentityUpdatesInternalStateUntilSnapshotCommit()
{
    std::vector<KiriView::DocumentSessionChange> changes;
    KiriView::DocumentSessionState state(
        [&changes](const std::vector<KiriView::DocumentSessionChange> &publishedChanges) {
            changes.insert(changes.end(), publishedChanges.cbegin(), publishedChanges.cend());
        });

    const QUrl url = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));
    state.setSourceIdentity(url);

    QCOMPARE(state.sourceUrl(), url);
    QCOMPARE(changes.size(), std::size_t(0));

    state.setSourceIdentity(url);
    QCOMPARE(changes.size(), std::size_t(0));
}

void TestDocumentSessionState::documentKindUpdatesInternalStateUntilSnapshotCommit()
{
    std::vector<std::vector<KiriView::DocumentSessionChange>> batches;
    KiriView::DocumentSessionState state(
        [&batches](const std::vector<KiriView::DocumentSessionChange> &changes) {
            batches.push_back(changes);
        });

    state.setDocumentKindAndActiveZoomSnapshot(KiriView::DocumentSessionKind::Video,
        KiriView::ActiveZoomSnapshot { true, true, 67, false });

    QCOMPARE(state.documentKind(), KiriView::DocumentSessionKind::Video);
    QCOMPARE(batches.size(), std::size_t(0));

    state.setDocumentKindAndActiveZoomSnapshot(KiriView::DocumentSessionKind::Video,
        KiriView::ActiveZoomSnapshot { true, true, 67, false });
    QCOMPARE(batches.size(), std::size_t(0));
}

void TestDocumentSessionState::activeZoomReadoutPublishesThroughSnapshotCommit()
{
    std::vector<KiriView::DocumentSessionChange> changes;
    KiriView::DocumentSessionState state(
        [&changes](const std::vector<KiriView::DocumentSessionChange> &publishedChanges) {
            changes.insert(changes.end(), publishedChanges.cbegin(), publishedChanges.cend());
        });

    KiriView::DocumentSessionPublicSnapshotInput input;
    input.session.documentKind = KiriView::DocumentSessionKind::Image;
    input.image.zoomPercentKnown = true;
    input.image.zoomPercent = 125.0;
    state.updatePublicSnapshot(input);

    QVERIFY(state.activeZoomSnapshot().available);
    QVERIFY(state.activeZoomSnapshot().known);
    QCOMPARE(state.activeZoomSnapshot().percent, 125.0);
    QCOMPARE(changes.size(), std::size_t(3));
    QCOMPARE(changes.at(0), KiriView::DocumentSessionChange::PublicProjectionRevision);
    QCOMPARE(changes.at(2), KiriView::DocumentSessionChange::ActiveZoomReadout);

    state.updatePublicSnapshot(input);
    QCOMPARE(changes.size(), std::size_t(3));

    input.image.zoomPercent = 150.0;
    state.updatePublicSnapshot(input);
    QCOMPARE(changes.size(), std::size_t(5));
    QCOMPARE(changes.at(4), KiriView::DocumentSessionChange::ActiveZoomReadout);
    QCOMPARE(state.activeZoomSnapshot().percent, 150.0);
}

void TestDocumentSessionState::fileDeletionProgressPublishesThroughSnapshotCommit()
{
    std::vector<std::vector<KiriView::DocumentSessionChange>> batches;
    KiriView::DocumentSessionState state(
        [&batches](const std::vector<KiriView::DocumentSessionChange> &changes) {
            batches.push_back(changes);
        });

    state.setFileDeletionInProgress(true);

    QVERIFY(state.fileDeletionInProgress());
    QCOMPARE(batches.size(), std::size_t(0));

    KiriView::DocumentSessionPublicSnapshotInput input;
    input.session.fileDeletionInProgress = true;
    QVERIFY(state.updatePublicSnapshot(input));
    QCOMPARE(batches.size(), std::size_t(1));
    QCOMPARE(batches.back().size(), std::size_t(2));
    QCOMPARE(batches.back().at(0), KiriView::DocumentSessionChange::PublicProjectionRevision);
    QCOMPARE(batches.back().at(1), KiriView::DocumentSessionChange::FileDeletionInProgress);

    state.setFileDeletionInProgress(true);
    QCOMPARE(batches.size(), std::size_t(1));
}

void TestDocumentSessionState::directMediaNavigationSnapshotOwnsBoundaryAndCandidates()
{
    std::vector<KiriView::DocumentSessionChange> changes;
    KiriView::DocumentSessionState state(
        [&changes](const std::vector<KiriView::DocumentSessionChange> &publishedChanges) {
            changes.insert(changes.end(), publishedChanges.cbegin(), publishedChanges.cend());
        });

    KiriView::DirectMediaNavigationBoundaryState boundary {
        true,
        false,
        false,
        true,
        2,
        2,
    };
    const QUrl firstUrl = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = QUrl::fromLocalFile(QStringLiteral("/media/02.mp4"));
    state.setDirectMediaNavigation(boundary, true,
        {
            directMediaNavigationCandidate(firstUrl),
            directMediaNavigationCandidate(secondUrl),
        });

    QVERIFY(state.directMediaNavigationKnown());
    QCOMPARE(state.directMediaNavigationState().currentNumber, 2);
    QCOMPARE(state.directMediaNavigationCandidates().size(), std::size_t(2));
    QCOMPARE(state.directMediaNavigationCandidates().at(0).url, firstUrl);
    QCOMPARE(state.directMediaNavigationCandidates().at(1).url, secondUrl);
    QCOMPARE(changes.size(), std::size_t(0));

    state.setDirectMediaNavigation(boundary, true,
        {
            directMediaNavigationCandidate(firstUrl),
            directMediaNavigationCandidate(secondUrl),
        });
    QCOMPARE(changes.size(), std::size_t(0));

    boundary.canOpenNext = true;
    state.setDirectMediaNavigation(boundary, true,
        {
            directMediaNavigationCandidate(firstUrl),
            directMediaNavigationCandidate(secondUrl),
        });
    QCOMPARE(state.directMediaNavigationState().canOpenNext, true);
    QCOMPARE(changes.size(), std::size_t(0));

    state.setDirectMediaNavigation({}, false, {});
    QVERIFY(!state.directMediaNavigationKnown());
    QCOMPARE(state.directMediaNavigationState().currentNumber, 0);
    QVERIFY(state.directMediaNavigationCandidates().empty());
}

void TestDocumentSessionState::publicProjectionCommitsValuesBeforePublishing()
{
    std::vector<std::vector<KiriView::DocumentSessionChange>> batches;
    KiriView::DocumentSessionState *stateDuringCallback = nullptr;
    KiriView::DocumentSessionState state(
        [&batches, &stateDuringCallback](
            const std::vector<KiriView::DocumentSessionChange> &publishedChanges) {
            batches.push_back(publishedChanges);
            QVERIFY(stateDuringCallback != nullptr);
            QCOMPARE(stateDuringCallback->activeNavigationSourceKind(),
                KiriView::ActiveNavigationSourceKind::ImageDocumentPages);
            QCOMPARE(stateDuringCallback->activeNavigationBoundaryScope(),
                KiriView::ActiveNavigationBoundaryScope::ImageDocumentPage);
            QCOMPARE(stateDuringCallback->activeNavigationSnapshot().currentNumber, 2);
            QCOMPARE(stateDuringCallback->activeNavigationSnapshot().count, 4);
            QCOMPARE(stateDuringCallback->windowTitleSubject(), QStringLiteral("book.cbz – 2/4"));
            QVERIFY(stateDuringCallback->displayedMediaOpenWithAvailable());
            QVERIFY(stateDuringCallback->displayedFileDeletionAvailable());
        });
    stateDuringCallback = &state;

    KiriView::DocumentSessionPublicProjectionInput input;
    input.documentKind = KiriView::DocumentSessionKind::Image;
    input.imageSourceMayRepresentDocument = true;
    input.imageDocumentPageNavigation = KiriView::ImageDocumentPageActiveNavigationSnapshot { true,
        true, true, false, false, 2, 4 };
    input.imageWindowTitleFileName = QStringLiteral("book.cbz");
    input.imageReadyForDeletion = true;
    input.displayedMediaOpenWithAvailable = true;

    state.updatePublicProjection(input);

    QCOMPARE(batches.size(), std::size_t(1));
    QCOMPARE(batches.back().size(), std::size_t(4));
    QCOMPARE(batches.back().at(0), KiriView::DocumentSessionChange::ActiveNavigation);
    QCOMPARE(batches.back().at(1), KiriView::DocumentSessionChange::WindowTitleSubject);
    QCOMPARE(batches.back().at(2), KiriView::DocumentSessionChange::FileDeletionAvailability);
    QCOMPARE(batches.back().at(3), KiriView::DocumentSessionChange::OpenWithAvailability);

    state.updatePublicProjection(input);
    QCOMPARE(batches.size(), std::size_t(1));
}

void TestDocumentSessionState::publicProjectionOnlyNotifiesChangedOutputs()
{
    std::vector<std::vector<KiriView::DocumentSessionChange>> batches;
    KiriView::DocumentSessionState state(
        [&batches](const std::vector<KiriView::DocumentSessionChange> &changes) {
            batches.push_back(changes);
        });

    KiriView::DocumentSessionPublicProjectionInput input;
    input.documentKind = KiriView::DocumentSessionKind::Video;
    input.directMediaNavigation = KiriView::DirectMediaActiveNavigationInput {
        KiriView::DirectMediaNavigationBoundaryState { false, false, true, true, 1, 1 },
        true,
    };
    input.videoSourcePresent = true;
    input.displayedMediaOpenWithAvailable = true;

    state.updatePublicProjection(input);
    QCOMPARE(batches.size(), std::size_t(1));

    input.documentKind = KiriView::DocumentSessionKind::Image;
    input.directMediaNavigation = {};
    input.imageSourceMayRepresentDocument = true;
    input.imageDocumentPageNavigation = KiriView::ImageDocumentPageActiveNavigationSnapshot { true,
        false, false, true, true, 1, 1 };
    input.imageReadyForDeletion = true;
    input.videoSourcePresent = false;
    state.updatePublicProjection(input);

    QCOMPARE(batches.size(), std::size_t(2));
    QCOMPARE(batches.back().size(), std::size_t(1));
    QCOMPARE(batches.back().at(0), KiriView::DocumentSessionChange::ActiveNavigation);
    QCOMPARE(state.activeNavigationBoundaryScope(),
        KiriView::ActiveNavigationBoundaryScope::ImageDocumentPage);

    input.imageReadyForDeletion = false;
    state.updatePublicProjection(input);

    QCOMPARE(batches.size(), std::size_t(3));
    QCOMPARE(batches.back().size(), std::size_t(1));
    QCOMPARE(batches.back().at(0), KiriView::DocumentSessionChange::FileDeletionAvailability);
    QVERIFY(!state.displayedFileDeletionAvailable());

    input.displayedMediaOpenWithAvailable = false;
    state.updatePublicProjection(input);

    QCOMPARE(batches.size(), std::size_t(4));
    QCOMPARE(batches.back().size(), std::size_t(1));
    QCOMPARE(batches.back().at(0), KiriView::DocumentSessionChange::OpenWithAvailability);
    QVERIFY(!state.displayedMediaOpenWithAvailable());

    state.updatePublicProjection(input);
    QCOMPARE(batches.size(), std::size_t(4));
}

void TestDocumentSessionState::publicSnapshotCommitsOneRevisionedBatch()
{
    std::vector<std::vector<KiriView::DocumentSessionChange>> batches;
    KiriView::DocumentSessionState *stateDuringCallback = nullptr;
    KiriView::DocumentSessionState state(
        [&batches, &stateDuringCallback](
            const std::vector<KiriView::DocumentSessionChange> &publishedChanges) {
            batches.push_back(publishedChanges);
            QVERIFY(stateDuringCallback != nullptr);
            QCOMPARE(stateDuringCallback->publicSnapshot().revision, quint64(1));
            QCOMPARE(stateDuringCallback->sourceUrl(),
                QUrl::fromLocalFile(QStringLiteral("/media/01.png")));
            QCOMPARE(stateDuringCallback->documentKind(), KiriView::DocumentSessionKind::Image);
            QCOMPARE(stateDuringCallback->activeZoomSnapshot().percent, 125.0);
            QCOMPARE(stateDuringCallback->activeNavigationSnapshot().currentNumber, 1);
            QCOMPARE(stateDuringCallback->windowTitleSubject(), QStringLiteral("01.png – 640×480"));
            QVERIFY(stateDuringCallback->displayedFileDeletionAvailable());
            QVERIFY(stateDuringCallback->displayedMediaOpenWithAvailable());
        });
    stateDuringCallback = &state;

    KiriView::DocumentSessionPublicSnapshotInput input;
    input.inputRevision = 12;
    input.session.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));
    input.session.documentKind = KiriView::DocumentSessionKind::Image;
    input.session.directImageLoadMayUseImageDocumentSourceScope = true;
    input.session.directMediaNavigation = KiriView::DirectMediaActiveNavigationInput {
        KiriView::DirectMediaNavigationBoundaryState { false, false, true, true, 1, 1 },
        true,
    };
    input.image.windowTitleFileName = QStringLiteral("01.png");
    input.image.directMediaSize = QSize(640, 480);
    input.image.readyForDeletion = true;
    input.image.zoomPercentKnown = true;
    input.image.zoomPercent = 125.0;
    input.operations.displayedMediaOpenWithAvailable = true;

    QVERIFY(state.updatePublicSnapshot(input));

    QCOMPARE(batches.size(), std::size_t(1));
    QCOMPARE(batches.back().size(), std::size_t(8));
    QCOMPARE(batches.back().at(0), KiriView::DocumentSessionChange::PublicProjectionRevision);
    QCOMPARE(batches.back().at(1), KiriView::DocumentSessionChange::SourceUrl);
    QCOMPARE(batches.back().at(2), KiriView::DocumentSessionChange::DocumentKind);
    QCOMPARE(batches.back().at(3), KiriView::DocumentSessionChange::ActiveZoomReadout);
    QCOMPARE(batches.back().at(4), KiriView::DocumentSessionChange::ActiveNavigation);
    QCOMPARE(batches.back().at(5), KiriView::DocumentSessionChange::WindowTitleSubject);
    QCOMPARE(batches.back().at(6), KiriView::DocumentSessionChange::FileDeletionAvailability);
    QCOMPARE(batches.back().at(7), KiriView::DocumentSessionChange::OpenWithAvailability);
    QCOMPARE(state.publicSnapshot().inputRevision, quint64(12));
}

void TestDocumentSessionState::unchangedPublicSnapshotDoesNotAdvanceRevision()
{
    std::vector<std::vector<KiriView::DocumentSessionChange>> batches;
    KiriView::DocumentSessionState state(
        [&batches](const std::vector<KiriView::DocumentSessionChange> &changes) {
            batches.push_back(changes);
        });

    KiriView::DocumentSessionPublicSnapshotInput input;
    input.inputRevision = 3;
    input.session.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/clip.mp4"));
    input.session.documentKind = KiriView::DocumentSessionKind::Video;
    input.video.sourcePresent = true;

    QVERIFY(state.updatePublicSnapshot(input));
    QCOMPARE(state.publicSnapshot().revision, quint64(1));
    QVERIFY(!state.updatePublicSnapshot(input));
    QCOMPARE(state.publicSnapshot().revision, quint64(1));
    QCOMPARE(batches.size(), std::size_t(1));
}

void TestDocumentSessionState::publishDeduplicatesChangesInOrder()
{
    std::vector<KiriView::DocumentSessionChange> changes;
    KiriView::DocumentSessionState state(
        [&changes](const std::vector<KiriView::DocumentSessionChange> &publishedChanges) {
            changes = publishedChanges;
        });

    state.publish({ KiriView::DocumentSessionChange::ErrorString,
        KiriView::DocumentSessionChange::SourceUrl, KiriView::DocumentSessionChange::ErrorString });

    QCOMPARE(changes.size(), std::size_t(2));
    QCOMPARE(changes.at(0), KiriView::DocumentSessionChange::ErrorString);
    QCOMPARE(changes.at(1), KiriView::DocumentSessionChange::SourceUrl);
}

QTEST_GUILESS_MAIN(TestDocumentSessionState)

#include "test_documentsessionstate.moc"
