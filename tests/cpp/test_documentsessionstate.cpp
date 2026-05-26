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
    void fileDeletionProgressOnlyPublishesProgress();
    void mediaNavigationStateOnlyUpdatesWhenBoundaryChanges();
    void activeNavigationSnapshotOnlyNotifiesWhenProjectionChanges();
    void publicProjectionCommitsValuesBeforePublishing();
    void publicProjectionOnlyNotifiesChangedOutputs();
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
    QCOMPARE(batches.back().size(), std::size_t(3));
    QCOMPARE(batches.back().at(0), KiriView::DocumentSessionChange::DocumentKind);
    QCOMPARE(batches.back().at(1), KiriView::DocumentSessionChange::ActiveZoomReadout);
    QCOMPARE(batches.back().at(2), KiriView::DocumentSessionChange::ErrorString);

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

void TestDocumentSessionState::fileDeletionProgressOnlyPublishesProgress()
{
    std::vector<std::vector<KiriView::DocumentSessionChange>> batches;
    KiriView::DocumentSessionState state(
        [&batches](const std::vector<KiriView::DocumentSessionChange> &changes) {
            batches.push_back(changes);
        });

    state.setFileDeletionInProgress(true);

    QVERIFY(state.fileDeletionInProgress());
    QCOMPARE(batches.size(), std::size_t(1));
    QCOMPARE(batches.back().size(), std::size_t(1));
    QCOMPARE(batches.back().at(0), KiriView::DocumentSessionChange::FileDeletionInProgress);

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
                KiriView::ActiveNavigationBoundaryScope::ImageDocument);
            QCOMPARE(stateDuringCallback->activeNavigationSnapshot().currentNumber, 2);
            QCOMPARE(stateDuringCallback->activeNavigationSnapshot().count, 4);
            QCOMPARE(stateDuringCallback->windowTitleSubject(), QStringLiteral("book.cbz – 2/4"));
            QVERIFY(stateDuringCallback->displayedFileDeletionAvailable());
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

    state.setPublicProjection(KiriView::DocumentSessionPublicProjection {
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages,
        KiriView::ActiveNavigationBoundaryScope::ImageDocument,
        snapshot,
        QStringLiteral("book.cbz – 2/4"),
        true,
    });

    QCOMPARE(batches.size(), std::size_t(1));
    QCOMPARE(batches.back().size(), std::size_t(3));
    QCOMPARE(batches.back().at(0), KiriView::DocumentSessionChange::ActiveNavigation);
    QCOMPARE(batches.back().at(1), KiriView::DocumentSessionChange::WindowTitleSubject);
    QCOMPARE(batches.back().at(2), KiriView::DocumentSessionChange::FileDeletionAvailability);

    state.setPublicProjection(KiriView::DocumentSessionPublicProjection {
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages,
        KiriView::ActiveNavigationBoundaryScope::ImageDocument,
        snapshot,
        QStringLiteral("book.cbz – 2/4"),
        true,
    });
    QCOMPARE(batches.size(), std::size_t(1));
}

void TestDocumentSessionState::publicProjectionOnlyNotifiesChangedOutputs()
{
    std::vector<std::vector<KiriView::DocumentSessionChange>> batches;
    KiriView::DocumentSessionState state(
        [&batches](const std::vector<KiriView::DocumentSessionChange> &changes) {
            batches.push_back(changes);
        });

    KiriView::ActiveNavigationSnapshot snapshot;
    snapshot.available = true;
    snapshot.known = true;
    snapshot.editable = true;
    snapshot.currentNumber = 1;
    snapshot.count = 1;

    KiriView::DocumentSessionPublicProjection projection {
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        KiriView::ActiveNavigationBoundaryScope::Media,
        snapshot,
        QStringLiteral("clip.mp4 – 1920×1080"),
        true,
    };

    state.setPublicProjection(projection);
    QCOMPARE(batches.size(), std::size_t(1));

    projection.sourceKind = KiriView::ActiveNavigationSourceKind::ImageDocumentPages;
    projection.boundaryScope = KiriView::ActiveNavigationBoundaryScope::ImageDocument;
    state.setPublicProjection(projection);

    QCOMPARE(batches.size(), std::size_t(2));
    QCOMPARE(batches.back().size(), std::size_t(1));
    QCOMPARE(batches.back().at(0), KiriView::DocumentSessionChange::ActiveNavigation);
    QCOMPARE(state.activeNavigationBoundaryScope(),
        KiriView::ActiveNavigationBoundaryScope::ImageDocument);

    projection.displayedFileDeletionAvailable = false;
    state.setPublicProjection(projection);

    QCOMPARE(batches.size(), std::size_t(3));
    QCOMPARE(batches.back().size(), std::size_t(1));
    QCOMPARE(batches.back().at(0), KiriView::DocumentSessionChange::FileDeletionAvailability);
    QVERIFY(!state.displayedFileDeletionAvailable());

    state.setPublicProjection(projection);
    QCOMPARE(batches.size(), std::size_t(3));
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
