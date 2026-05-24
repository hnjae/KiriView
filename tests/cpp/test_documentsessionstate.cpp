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
    void documentKindPublishesDerivedPublicChanges();
    void windowTitleSubjectOnlyNotifiesWhenChanged();
    void fileDeletionProgressPublishesProgressAndAvailability();
    void mediaNavigationStateOnlyUpdatesWhenBoundaryChanges();
    void activeNavigationSnapshotOnlyNotifiesWhenProjectionChanges();
    void directMediaCursorGenerationChangesOnlyWithEffectiveIdentity();
    void directMediaCursorGenerationUsesNormalizedEffectiveIdentity();
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

void TestDocumentSessionState::documentKindPublishesDerivedPublicChanges()
{
    std::vector<std::vector<KiriView::DocumentSessionChange>> batches;
    KiriView::DocumentSessionState state(
        [&batches](const std::vector<KiriView::DocumentSessionChange> &changes) {
            batches.push_back(changes);
        });

    state.setDocumentKind(KiriView::DocumentSessionKind::Video);

    QCOMPARE(state.documentKind(), KiriView::DocumentSessionKind::Video);
    QCOMPARE(batches.size(), std::size_t(1));
    QCOMPARE(batches.back().size(), std::size_t(4));
    QCOMPARE(batches.back().at(0), KiriView::DocumentSessionChange::DocumentKind);
    QCOMPARE(batches.back().at(1), KiriView::DocumentSessionChange::ActiveZoomReadout);
    QCOMPARE(batches.back().at(2), KiriView::DocumentSessionChange::ErrorString);
    QCOMPARE(batches.back().at(3), KiriView::DocumentSessionChange::FileDeletionAvailability);

    state.setDocumentKind(KiriView::DocumentSessionKind::Video);
    QCOMPARE(batches.size(), std::size_t(1));
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

void TestDocumentSessionState::directMediaCursorGenerationChangesOnlyWithEffectiveIdentity()
{
    KiriView::DocumentSessionState state;
    const QUrl firstImage = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));
    const QUrl secondImage = QUrl::fromLocalFile(QStringLiteral("/media/02.png"));
    const QUrl video = QUrl::fromLocalFile(QStringLiteral("/media/03.mp4"));

    const quint64 initialGeneration = state.directMediaCursor().generation;

    state.requestDirectImageCursor(firstImage);
    QCOMPARE(state.directMediaCursorUrl(), firstImage);
    QCOMPARE(state.directMediaCursor().pendingUrl, firstImage);
    QCOMPARE(state.directMediaCursor().stableUrl, QUrl());
    const quint64 requestedGeneration = state.directMediaCursor().generation;
    QVERIFY(requestedGeneration > initialGeneration);

    state.requestDirectImageCursor(firstImage);
    QCOMPARE(state.directMediaCursorUrl(), firstImage);
    QCOMPARE(state.directMediaCursor().generation, requestedGeneration);

    state.confirmDirectImageCursor(firstImage);
    QCOMPARE(state.directMediaCursorUrl(), firstImage);
    QCOMPARE(state.directMediaCursor().pendingUrl, QUrl());
    QCOMPARE(state.directMediaCursor().stableUrl, firstImage);
    const quint64 confirmedGeneration = state.directMediaCursor().generation;
    QCOMPARE(confirmedGeneration, requestedGeneration);

    state.confirmDirectImageCursor(firstImage);
    QCOMPARE(state.directMediaCursorUrl(), firstImage);
    QCOMPARE(state.directMediaCursor().generation, confirmedGeneration);

    state.requestDirectImageCursor(secondImage);
    QCOMPARE(state.directMediaCursorUrl(), secondImage);
    QCOMPARE(state.directMediaCursor().stableUrl, firstImage);
    QCOMPARE(state.directMediaCursor().pendingUrl, secondImage);
    const quint64 replacementGeneration = state.directMediaCursor().generation;
    QVERIFY(replacementGeneration > confirmedGeneration);

    state.requestDirectImageCursor(secondImage);
    QCOMPARE(state.directMediaCursorUrl(), secondImage);
    QCOMPARE(state.directMediaCursor().generation, replacementGeneration);

    state.restoreDirectImageCursorAfterFailure();
    QCOMPARE(state.directMediaCursorUrl(), firstImage);
    QCOMPARE(state.directMediaCursor().pendingUrl, QUrl());
    QCOMPARE(state.directMediaCursor().stableUrl, firstImage);
    const quint64 restoredGeneration = state.directMediaCursor().generation;
    QVERIFY(restoredGeneration > replacementGeneration);

    state.restoreDirectImageCursorAfterFailure();
    QCOMPARE(state.directMediaCursorUrl(), firstImage);
    QCOMPARE(state.directMediaCursor().generation, restoredGeneration);

    state.setDirectVideoCursor(video);
    QCOMPARE(state.directMediaCursorUrl(), video);
    QCOMPARE(state.directMediaCursor().stableUrl, video);
    const quint64 videoGeneration = state.directMediaCursor().generation;
    QVERIFY(videoGeneration > restoredGeneration);

    state.setDirectVideoCursor(video);
    QCOMPARE(state.directMediaCursorUrl(), video);
    QCOMPARE(state.directMediaCursor().generation, videoGeneration);

    state.clearDirectMediaCursor();
    QCOMPARE(state.directMediaCursorUrl(), QUrl());
    QCOMPARE(state.directMediaCursor().stableUrl, QUrl());
    const quint64 clearedGeneration = state.directMediaCursor().generation;
    QVERIFY(clearedGeneration > videoGeneration);

    state.clearDirectMediaCursor();
    QCOMPARE(state.directMediaCursorUrl(), QUrl());
    QCOMPARE(state.directMediaCursor().generation, clearedGeneration);
}

void TestDocumentSessionState::directMediaCursorGenerationUsesNormalizedEffectiveIdentity()
{
    KiriView::DocumentSessionState state;
    const QUrl requestedImage(QStringLiteral("file:///media/chapter/../01.png"));
    const QUrl displayedImage(QStringLiteral("file:///media/01.png"));
    const QUrl replacementImage(QStringLiteral("file:///media/02.png"));

    state.requestDirectImageCursor(requestedImage);
    const quint64 requestedGeneration = state.directMediaCursor().generation;

    state.confirmDirectImageCursor(displayedImage);
    QCOMPARE(state.directMediaCursorUrl(), displayedImage);
    QCOMPARE(state.directMediaCursor().generation, requestedGeneration);

    state.requestDirectImageCursor(replacementImage);
    QVERIFY(state.directMediaCursor().generation > requestedGeneration);
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
