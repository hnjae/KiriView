// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionstate.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <vector>

class TestDocumentSessionState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sourceIdentityOnlyNotifiesWhenChanged();
    void documentKindPublishesConsistentActiveZoomSnapshot();
    void activeZoomSnapshotOnlyNotifiesWhenProjectionChanges();
    void windowTitleSubjectOnlyNotifiesWhenChanged();
    void fileDeletionProgressPublishesProgressAndAvailability();
    void mediaNavigationStateOnlyUpdatesWhenBoundaryChanges();
    void activeNavigationSnapshotOnlyNotifiesWhenProjectionChanges();
    void activeNavigationProjectionCommitsTitleBeforePublishing();
    void publishDeduplicatesChangesInOrder();
};

void TestDocumentSessionState::sourceIdentityOnlyNotifiesWhenChanged()
{
    std::vector<KiriView::DocumentSessionChange> changes;
    KiriView::DocumentSessionState state(
        [&changes](const std::vector<KiriView::DocumentSessionChange> &publishedChanges) {
            changes.insert(changes.end(), publishedChanges.cbegin(), publishedChanges.cend());
        });

    const QUrl url = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));
    state.setSourceIdentity(url);

    QCOMPARE(state.sourceUrl(), url);
    QCOMPARE(changes.size(), std::size_t(1));
    QCOMPARE(changes.back(), KiriView::DocumentSessionChange::SourceUrl);

    state.setSourceIdentity(url);
    QCOMPARE(changes.size(), std::size_t(1));
}

void TestDocumentSessionState::documentKindPublishesConsistentActiveZoomSnapshot()
{
    std::vector<std::vector<KiriView::DocumentSessionChange>> batches;
    KiriView::DocumentSessionState state(
        [&batches](const std::vector<KiriView::DocumentSessionChange> &changes) {
            batches.push_back(changes);
        });

    state.setDocumentKindAndActiveZoomSnapshot(KiriView::DocumentSessionKind::Video,
        KiriView::ActiveZoomSnapshot { true, true, 67, false });

    QCOMPARE(state.documentKind(), KiriView::DocumentSessionKind::Video);
    QCOMPARE(state.activeZoomSnapshot().available, true);
    QCOMPARE(state.activeZoomSnapshot().known, true);
    QCOMPARE(state.activeZoomSnapshot().percent, 67.0);
    QCOMPARE(state.activeZoomSnapshot().editable, false);
    QCOMPARE(batches.size(), std::size_t(1));
    QCOMPARE(batches.back().size(), std::size_t(4));
    QCOMPARE(batches.back().at(0), KiriView::DocumentSessionChange::DocumentKind);
    QCOMPARE(batches.back().at(1), KiriView::DocumentSessionChange::ActiveZoomReadout);
    QCOMPARE(batches.back().at(2), KiriView::DocumentSessionChange::ErrorString);
    QCOMPARE(batches.back().at(3), KiriView::DocumentSessionChange::FileDeletionAvailability);

    state.setDocumentKindAndActiveZoomSnapshot(KiriView::DocumentSessionKind::Video,
        KiriView::ActiveZoomSnapshot { true, true, 67, false });
    QCOMPARE(batches.size(), std::size_t(1));
}

void TestDocumentSessionState::activeZoomSnapshotOnlyNotifiesWhenProjectionChanges()
{
    std::vector<KiriView::DocumentSessionChange> changes;
    KiriView::DocumentSessionState state(
        [&changes](const std::vector<KiriView::DocumentSessionChange> &publishedChanges) {
            changes.insert(changes.end(), publishedChanges.cbegin(), publishedChanges.cend());
        });

    KiriView::ActiveZoomSnapshot snapshot { true, false, 0.0, false };
    state.setActiveZoomSnapshot(snapshot);

    QVERIFY(state.activeZoomSnapshot().available);
    QVERIFY(!state.activeZoomSnapshot().known);
    QCOMPARE(changes.size(), std::size_t(1));
    QCOMPARE(changes.at(0), KiriView::DocumentSessionChange::ActiveZoomReadout);

    state.setActiveZoomSnapshot(snapshot);
    QCOMPARE(changes.size(), std::size_t(1));

    snapshot.known = true;
    snapshot.percent = 125.0;
    snapshot.editable = true;
    state.setActiveZoomSnapshot(snapshot);
    QCOMPARE(changes.size(), std::size_t(2));
    QCOMPARE(state.activeZoomSnapshot().percent, 125.0);
    QVERIFY(state.activeZoomSnapshot().editable);
}

void TestDocumentSessionState::windowTitleSubjectOnlyNotifiesWhenChanged()
{
    std::vector<KiriView::DocumentSessionChange> changes;
    KiriView::DocumentSessionState state(
        [&changes](const std::vector<KiriView::DocumentSessionChange> &publishedChanges) {
            changes.insert(changes.end(), publishedChanges.cbegin(), publishedChanges.cend());
        });

    state.setWindowTitleSubject(QStringLiteral("clip.mp4 – 1920×1080"));

    QCOMPARE(state.windowTitleSubject(), QStringLiteral("clip.mp4 – 1920×1080"));
    QCOMPARE(changes.size(), std::size_t(1));
    QCOMPARE(changes.at(0), KiriView::DocumentSessionChange::WindowTitleSubject);

    state.setWindowTitleSubject(QStringLiteral("clip.mp4 – 1920×1080"));
    QCOMPARE(changes.size(), std::size_t(1));
}

void TestDocumentSessionState::fileDeletionProgressPublishesProgressAndAvailability()
{
    std::vector<std::vector<KiriView::DocumentSessionChange>> batches;
    KiriView::DocumentSessionState state(
        [&batches](const std::vector<KiriView::DocumentSessionChange> &changes) {
            batches.push_back(changes);
        });

    state.setFileDeletionInProgress(true);

    QVERIFY(state.fileDeletionInProgress());
    QCOMPARE(batches.size(), std::size_t(1));
    QCOMPARE(batches.back().size(), std::size_t(2));
    QCOMPARE(batches.back().at(0), KiriView::DocumentSessionChange::FileDeletionInProgress);
    QCOMPARE(batches.back().at(1), KiriView::DocumentSessionChange::FileDeletionAvailability);

    state.setFileDeletionInProgress(true);
    QCOMPARE(batches.size(), std::size_t(1));
}

void TestDocumentSessionState::mediaNavigationStateOnlyUpdatesWhenBoundaryChanges()
{
    std::vector<KiriView::DocumentSessionChange> changes;
    KiriView::DocumentSessionState state(
        [&changes](const std::vector<KiriView::DocumentSessionChange> &publishedChanges) {
            changes.insert(changes.end(), publishedChanges.cbegin(), publishedChanges.cend());
        });

    KiriView::MediaNavigationBoundaryState boundary {
        true,
        false,
        false,
        true,
        2,
        2,
    };
    state.setMediaNavigationState(boundary, true);

    QVERIFY(state.mediaNavigationKnown());
    QCOMPARE(state.mediaNavigationState().currentNumber, 2);
    QCOMPARE(changes.size(), std::size_t(0));

    state.setMediaNavigationState(boundary, true);
    QCOMPARE(changes.size(), std::size_t(0));

    boundary.canOpenNext = true;
    state.setMediaNavigationState(boundary, true);
    QCOMPARE(state.mediaNavigationState().canOpenNext, true);
    QCOMPARE(changes.size(), std::size_t(0));
}

void TestDocumentSessionState::activeNavigationSnapshotOnlyNotifiesWhenProjectionChanges()
{
    std::vector<KiriView::DocumentSessionChange> changes;
    KiriView::DocumentSessionState state(
        [&changes](const std::vector<KiriView::DocumentSessionChange> &publishedChanges) {
            changes.insert(changes.end(), publishedChanges.cbegin(), publishedChanges.cend());
        });

    KiriView::ActiveNavigationSnapshot snapshot;
    snapshot.available = true;
    state.setActiveNavigationSnapshot(snapshot);

    QVERIFY(state.activeNavigationSnapshot().available);
    QCOMPARE(changes.size(), std::size_t(1));
    QCOMPARE(changes.at(0), KiriView::DocumentSessionChange::ActiveNavigation);

    state.setActiveNavigationSnapshot(snapshot);
    QCOMPARE(changes.size(), std::size_t(1));

    snapshot.known = true;
    snapshot.editable = true;
    snapshot.currentNumber = 1;
    snapshot.count = 1;
    state.setActiveNavigationSnapshot(snapshot);
    QCOMPARE(changes.size(), std::size_t(2));
    QVERIFY(state.activeNavigationSnapshot().known);
}

void TestDocumentSessionState::activeNavigationProjectionCommitsTitleBeforePublishing()
{
    std::vector<std::vector<KiriView::DocumentSessionChange>> batches;
    KiriView::DocumentSessionState *stateDuringCallback = nullptr;
    KiriView::DocumentSessionState state(
        [&batches, &stateDuringCallback](
            const std::vector<KiriView::DocumentSessionChange> &publishedChanges) {
            batches.push_back(publishedChanges);
            QVERIFY(stateDuringCallback != nullptr);
            QCOMPARE(stateDuringCallback->activeNavigationSnapshot().currentNumber, 2);
            QCOMPARE(stateDuringCallback->activeNavigationSnapshot().count, 4);
            QCOMPARE(stateDuringCallback->windowTitleSubject(), QStringLiteral("book.cbz – 2/4"));
        });
    stateDuringCallback = &state;

    KiriView::ActiveNavigationSnapshot snapshot;
    snapshot.available = true;
    snapshot.known = true;
    snapshot.editable = true;
    snapshot.canOpenPrevious = true;
    snapshot.canOpenNext = true;
    snapshot.currentNumber = 2;
    snapshot.count = 4;

    state.setActiveNavigationProjection(snapshot, QStringLiteral("book.cbz – 2/4"));

    QCOMPARE(batches.size(), std::size_t(1));
    QCOMPARE(batches.back().size(), std::size_t(2));
    QCOMPARE(batches.back().at(0), KiriView::DocumentSessionChange::ActiveNavigation);
    QCOMPARE(batches.back().at(1), KiriView::DocumentSessionChange::WindowTitleSubject);

    state.setActiveNavigationProjection(snapshot, QStringLiteral("book.cbz – 2/4"));
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
