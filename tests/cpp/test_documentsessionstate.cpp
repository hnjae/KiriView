// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionstate.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>
#include <algorithm>
#include <vector>

namespace {
kiriview::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl& url)
{
    return kiriview::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

kiriview::DocumentSessionPublicSnapshotInput snapshotInputForProjection(
    const kiriview::DocumentSessionPublicProjectionInput& input)
{
    kiriview::DocumentSessionPublicSnapshotInput snapshotInput;
    snapshotInput.session.documentKind = input.documentKind;
    snapshotInput.session.directImageLoadMayUseImageDocumentSourceScope
        = input.directImageLoadMayUseImageDocumentSourceScope;
    snapshotInput.session.fileDeletionInProgress = input.fileDeletionInProgress;
    snapshotInput.session.directMediaNavigation = input.directMediaNavigation;
    snapshotInput.image.sourceMayRepresentDocument = input.imageSourceMayRepresentDocument;
    snapshotInput.image.pageNavigation = input.imageDocumentPageNavigation;
    snapshotInput.image.windowTitleFileName = input.imageWindowTitleFileName;
    snapshotInput.image.directMediaSize = input.imageDirectMediaSize;
    snapshotInput.image.readyForDeletion = input.imageReadyForDeletion;
    snapshotInput.image.directImageReplacementPending = input.directImageReplacementPending;
    snapshotInput.video.windowTitleFileName = input.videoWindowTitleFileName;
    snapshotInput.video.directMediaSize = input.videoDirectMediaSize;
    snapshotInput.video.sourcePresent = input.videoSourcePresent;
    snapshotInput.video.error = input.videoError;
    snapshotInput.operations.displayedMediaOpenWithAvailable
        = input.displayedMediaOpenWithAvailable;
    return snapshotInput;
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
    void publicSnapshotCommitsProjectionValuesBeforePublishing();
    void publicSnapshotOnlyNotifiesChangedProjectionOutputs();
    void publicSnapshotCommitsOneRevisionedBatch();
    void unchangedPublicSnapshotDoesNotAdvanceRevision();
    void publishDeduplicatesChangesInOrder();
};

void TestDocumentSessionState::sourceIdentityUpdatesInternalStateUntilSnapshotCommit()
{
    std::vector<kiriview::DocumentSessionChange> changes;
    kiriview::DocumentSessionState state(
        [&changes](const std::vector<kiriview::DocumentSessionChange>& publishedChanges) {
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
    std::vector<std::vector<kiriview::DocumentSessionChange>> batches;
    kiriview::DocumentSessionState state(
        [&batches](const std::vector<kiriview::DocumentSessionChange>& changes) {
            batches.push_back(changes);
        });

    state.setDocumentKindAndActiveZoomSnapshot(kiriview::DocumentSessionKind::Video,
        kiriview::ActiveZoomSnapshot { true, true, 67, false });

    QCOMPARE(state.documentKind(), kiriview::DocumentSessionKind::Video);
    QCOMPARE(batches.size(), std::size_t(0));

    state.setDocumentKindAndActiveZoomSnapshot(kiriview::DocumentSessionKind::Video,
        kiriview::ActiveZoomSnapshot { true, true, 67, false });
    QCOMPARE(batches.size(), std::size_t(0));
}

void TestDocumentSessionState::activeZoomReadoutPublishesThroughSnapshotCommit()
{
    std::vector<kiriview::DocumentSessionChange> changes;
    kiriview::DocumentSessionState state(
        [&changes](const std::vector<kiriview::DocumentSessionChange>& publishedChanges) {
            changes.insert(changes.end(), publishedChanges.cbegin(), publishedChanges.cend());
        });

    kiriview::DocumentSessionPublicSnapshotInput input;
    input.session.documentKind = kiriview::DocumentSessionKind::Image;
    input.image.zoomPercentKnown = true;
    input.image.zoomPercent = 125.0;
    state.updatePublicSnapshot(input);

    QVERIFY(state.activeZoomSnapshot().available);
    QVERIFY(state.activeZoomSnapshot().known);
    QCOMPARE(state.activeZoomSnapshot().percent, 125.0);
    QCOMPARE(changes.size(), std::size_t(3));
    QCOMPARE(changes.at(0), kiriview::DocumentSessionChange::PublicProjectionRevision);
    QCOMPARE(changes.at(2), kiriview::DocumentSessionChange::ActiveZoomReadout);

    state.updatePublicSnapshot(input);
    QCOMPARE(changes.size(), std::size_t(3));

    input.image.zoomPercent = 150.0;
    state.updatePublicSnapshot(input);
    QCOMPARE(changes.size(), std::size_t(5));
    QCOMPARE(changes.at(4), kiriview::DocumentSessionChange::ActiveZoomReadout);
    QCOMPARE(state.activeZoomSnapshot().percent, 150.0);
}

void TestDocumentSessionState::fileDeletionProgressPublishesThroughSnapshotCommit()
{
    std::vector<std::vector<kiriview::DocumentSessionChange>> batches;
    kiriview::DocumentSessionState state(
        [&batches](const std::vector<kiriview::DocumentSessionChange>& changes) {
            batches.push_back(changes);
        });

    state.setFileDeletionInProgress(true);

    QVERIFY(state.fileDeletionInProgress());
    QCOMPARE(batches.size(), std::size_t(0));

    kiriview::DocumentSessionPublicSnapshotInput input;
    input.session.fileDeletionInProgress = true;
    QVERIFY(state.updatePublicSnapshot(input));
    QCOMPARE(batches.size(), std::size_t(1));
    QCOMPARE(batches.back().size(), std::size_t(2));
    QCOMPARE(batches.back().at(0), kiriview::DocumentSessionChange::PublicProjectionRevision);
    QCOMPARE(batches.back().at(1), kiriview::DocumentSessionChange::FileDeletionInProgress);

    state.setFileDeletionInProgress(true);
    QCOMPARE(batches.size(), std::size_t(1));
}

void TestDocumentSessionState::directMediaNavigationSnapshotOwnsBoundaryAndCandidates()
{
    std::vector<kiriview::DocumentSessionChange> changes;
    kiriview::DocumentSessionState state(
        [&changes](const std::vector<kiriview::DocumentSessionChange>& publishedChanges) {
            changes.insert(changes.end(), publishedChanges.cbegin(), publishedChanges.cend());
        });

    kiriview::DirectMediaNavigationBoundaryState boundary {
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

void TestDocumentSessionState::publicSnapshotCommitsProjectionValuesBeforePublishing()
{
    std::vector<std::vector<kiriview::DocumentSessionChange>> batches;
    kiriview::DocumentSessionState* stateDuringCallback = nullptr;
    kiriview::DocumentSessionState state(
        [&batches, &stateDuringCallback](
            const std::vector<kiriview::DocumentSessionChange>& publishedChanges) {
            batches.push_back(publishedChanges);
            QVERIFY(stateDuringCallback != nullptr);
            QCOMPARE(stateDuringCallback->activeNavigationSourceKind(),
                kiriview::ActiveNavigationSourceKind::ImageDocumentPages);
            QCOMPARE(stateDuringCallback->activeNavigationBoundaryScope(),
                kiriview::ActiveNavigationBoundaryScope::ImageDocumentPage);
            QCOMPARE(stateDuringCallback->activeNavigationSnapshot().currentNumber, 2);
            QCOMPARE(stateDuringCallback->activeNavigationSnapshot().count, 4);
            QCOMPARE(stateDuringCallback->windowTitleSubject(), QStringLiteral("book.cbz – 2/4"));
            QVERIFY(stateDuringCallback->displayedMediaOpenWithAvailable());
            QVERIFY(stateDuringCallback->displayedFileDeletionAvailable());
        });
    stateDuringCallback = &state;

    kiriview::DocumentSessionPublicProjectionInput input;
    input.documentKind = kiriview::DocumentSessionKind::Image;
    input.imageSourceMayRepresentDocument = true;
    input.imageDocumentPageNavigation = kiriview::ImageDocumentPageActiveNavigationSnapshot { true,
        true, true, false, false, 2, 4 };
    input.imageWindowTitleFileName = QStringLiteral("book.cbz");
    input.imageReadyForDeletion = true;
    input.displayedMediaOpenWithAvailable = true;

    state.updatePublicSnapshot(snapshotInputForProjection(input));

    QCOMPARE(batches.size(), std::size_t(1));
    QCOMPARE(batches.back().size(), std::size_t(6));
    QCOMPARE(batches.back().at(0), kiriview::DocumentSessionChange::PublicProjectionRevision);
    QCOMPARE(batches.back().at(1), kiriview::DocumentSessionChange::DocumentKind);
    QCOMPARE(batches.back().at(2), kiriview::DocumentSessionChange::ActiveNavigation);
    QCOMPARE(batches.back().at(3), kiriview::DocumentSessionChange::WindowTitleSubject);
    QCOMPARE(batches.back().at(4), kiriview::DocumentSessionChange::FileDeletionAvailability);
    QCOMPARE(batches.back().at(5), kiriview::DocumentSessionChange::OpenWithAvailability);

    state.updatePublicSnapshot(snapshotInputForProjection(input));
    QCOMPARE(batches.size(), std::size_t(1));
}

void TestDocumentSessionState::publicSnapshotOnlyNotifiesChangedProjectionOutputs()
{
    std::vector<std::vector<kiriview::DocumentSessionChange>> batches;
    kiriview::DocumentSessionState state(
        [&batches](const std::vector<kiriview::DocumentSessionChange>& changes) {
            batches.push_back(changes);
        });

    kiriview::DocumentSessionPublicProjectionInput input;
    input.documentKind = kiriview::DocumentSessionKind::Video;
    input.directMediaNavigation = kiriview::DirectMediaActiveNavigationInput {
        kiriview::DirectMediaNavigationBoundaryState { false, false, true, true, 1, 1 },
        true,
    };
    input.videoSourcePresent = true;
    input.displayedMediaOpenWithAvailable = true;

    state.updatePublicSnapshot(snapshotInputForProjection(input));
    QCOMPARE(batches.size(), std::size_t(1));

    input.documentKind = kiriview::DocumentSessionKind::Image;
    input.directMediaNavigation = {};
    input.imageSourceMayRepresentDocument = true;
    input.imageDocumentPageNavigation = kiriview::ImageDocumentPageActiveNavigationSnapshot { true,
        false, false, true, true, 1, 1 };
    input.imageReadyForDeletion = true;
    input.videoSourcePresent = false;
    state.updatePublicSnapshot(snapshotInputForProjection(input));

    QCOMPARE(batches.size(), std::size_t(2));
    QVERIFY(std::find(batches.back().cbegin(), batches.back().cend(),
                kiriview::DocumentSessionChange::PublicProjectionRevision)
        != batches.back().cend());
    QVERIFY(std::find(batches.back().cbegin(), batches.back().cend(),
                kiriview::DocumentSessionChange::DocumentKind)
        != batches.back().cend());
    QVERIFY(std::find(batches.back().cbegin(), batches.back().cend(),
                kiriview::DocumentSessionChange::ActiveNavigation)
        != batches.back().cend());
    QCOMPARE(state.activeNavigationBoundaryScope(),
        kiriview::ActiveNavigationBoundaryScope::ImageDocumentPage);

    input.imageReadyForDeletion = false;
    state.updatePublicSnapshot(snapshotInputForProjection(input));

    QCOMPARE(batches.size(), std::size_t(3));
    QCOMPARE(batches.back().size(), std::size_t(2));
    QCOMPARE(batches.back().at(0), kiriview::DocumentSessionChange::PublicProjectionRevision);
    QCOMPARE(batches.back().at(1), kiriview::DocumentSessionChange::FileDeletionAvailability);
    QVERIFY(!state.displayedFileDeletionAvailable());

    input.displayedMediaOpenWithAvailable = false;
    state.updatePublicSnapshot(snapshotInputForProjection(input));

    QCOMPARE(batches.size(), std::size_t(4));
    QCOMPARE(batches.back().size(), std::size_t(2));
    QCOMPARE(batches.back().at(0), kiriview::DocumentSessionChange::PublicProjectionRevision);
    QCOMPARE(batches.back().at(1), kiriview::DocumentSessionChange::OpenWithAvailability);
    QVERIFY(!state.displayedMediaOpenWithAvailable());

    state.updatePublicSnapshot(snapshotInputForProjection(input));
    QCOMPARE(batches.size(), std::size_t(4));
}

void TestDocumentSessionState::publicSnapshotCommitsOneRevisionedBatch()
{
    std::vector<std::vector<kiriview::DocumentSessionChange>> batches;
    kiriview::DocumentSessionState* stateDuringCallback = nullptr;
    kiriview::DocumentSessionState state(
        [&batches, &stateDuringCallback](
            const std::vector<kiriview::DocumentSessionChange>& publishedChanges) {
            batches.push_back(publishedChanges);
            QVERIFY(stateDuringCallback != nullptr);
            QCOMPARE(stateDuringCallback->publicSnapshot().revision, quint64(1));
            QCOMPARE(stateDuringCallback->sourceUrl(),
                QUrl::fromLocalFile(QStringLiteral("/media/01.png")));
            QCOMPARE(stateDuringCallback->documentKind(), kiriview::DocumentSessionKind::Image);
            QCOMPARE(stateDuringCallback->activeZoomSnapshot().percent, 125.0);
            QCOMPARE(stateDuringCallback->activeNavigationSnapshot().currentNumber, 1);
            QCOMPARE(stateDuringCallback->windowTitleSubject(), QStringLiteral("01.png – 640×480"));
            QVERIFY(stateDuringCallback->displayedFileDeletionAvailable());
            QVERIFY(stateDuringCallback->displayedMediaOpenWithAvailable());
        });
    stateDuringCallback = &state;

    kiriview::DocumentSessionPublicSnapshotInput input;
    input.inputRevision = 12;
    input.session.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));
    input.session.documentKind = kiriview::DocumentSessionKind::Image;
    input.session.directImageLoadMayUseImageDocumentSourceScope = true;
    input.session.directMediaNavigation = kiriview::DirectMediaActiveNavigationInput {
        kiriview::DirectMediaNavigationBoundaryState { false, false, true, true, 1, 1 },
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
    QCOMPARE(batches.back().at(0), kiriview::DocumentSessionChange::PublicProjectionRevision);
    QCOMPARE(batches.back().at(1), kiriview::DocumentSessionChange::SourceUrl);
    QCOMPARE(batches.back().at(2), kiriview::DocumentSessionChange::DocumentKind);
    QCOMPARE(batches.back().at(3), kiriview::DocumentSessionChange::ActiveZoomReadout);
    QCOMPARE(batches.back().at(4), kiriview::DocumentSessionChange::ActiveNavigation);
    QCOMPARE(batches.back().at(5), kiriview::DocumentSessionChange::WindowTitleSubject);
    QCOMPARE(batches.back().at(6), kiriview::DocumentSessionChange::FileDeletionAvailability);
    QCOMPARE(batches.back().at(7), kiriview::DocumentSessionChange::OpenWithAvailability);
    QCOMPARE(state.publicSnapshot().inputRevision, quint64(12));
}

void TestDocumentSessionState::unchangedPublicSnapshotDoesNotAdvanceRevision()
{
    std::vector<std::vector<kiriview::DocumentSessionChange>> batches;
    kiriview::DocumentSessionState state(
        [&batches](const std::vector<kiriview::DocumentSessionChange>& changes) {
            batches.push_back(changes);
        });

    kiriview::DocumentSessionPublicSnapshotInput input;
    input.inputRevision = 3;
    input.session.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/clip.mp4"));
    input.session.documentKind = kiriview::DocumentSessionKind::Video;
    input.video.sourcePresent = true;

    QVERIFY(state.updatePublicSnapshot(input));
    QCOMPARE(state.publicSnapshot().revision, quint64(1));
    QVERIFY(!state.updatePublicSnapshot(input));
    QCOMPARE(state.publicSnapshot().revision, quint64(1));
    QCOMPARE(batches.size(), std::size_t(1));
}

void TestDocumentSessionState::publishDeduplicatesChangesInOrder()
{
    std::vector<kiriview::DocumentSessionChange> changes;
    kiriview::DocumentSessionState state(
        [&changes](const std::vector<kiriview::DocumentSessionChange>& publishedChanges) {
            changes = publishedChanges;
        });

    state.publish({ kiriview::DocumentSessionChange::ErrorString,
        kiriview::DocumentSessionChange::SourceUrl, kiriview::DocumentSessionChange::ErrorString });

    QCOMPARE(changes.size(), std::size_t(2));
    QCOMPARE(changes.at(0), kiriview::DocumentSessionChange::ErrorString);
    QCOMPARE(changes.at(1), kiriview::DocumentSessionChange::SourceUrl);
}

QTEST_GUILESS_MAIN(TestDocumentSessionState)

#include "test_documentsessionstate.moc"
