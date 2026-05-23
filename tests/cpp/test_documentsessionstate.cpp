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
    void fileDeletionProgressPublishesProgressAndAvailability();
    void mediaNavigationStateOnlyNotifiesWhenBoundaryChanges();
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
    QCOMPARE(batches.back().size(), std::size_t(7));
    QCOMPARE(batches.back().at(0), KiriView::DocumentSessionChange::DocumentKind);
    QCOMPARE(batches.back().at(1), KiriView::DocumentSessionChange::ActiveZoomReadout);
    QCOMPARE(batches.back().at(2), KiriView::DocumentSessionChange::WindowTitleFileName);
    QCOMPARE(batches.back().at(3), KiriView::DocumentSessionChange::ErrorString);
    QCOMPARE(batches.back().at(4), KiriView::DocumentSessionChange::FileDeletionAvailability);
    QCOMPARE(batches.back().at(5), KiriView::DocumentSessionChange::MediaNavigationAvailability);
    QCOMPARE(batches.back().at(6), KiriView::DocumentSessionChange::ActiveNavigation);

    state.setDocumentKind(KiriView::DocumentSessionKind::Video);
    QCOMPARE(batches.size(), std::size_t(1));
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

void TestDocumentSessionState::mediaNavigationStateOnlyNotifiesWhenBoundaryChanges()
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
    QCOMPARE(changes.size(), std::size_t(2));
    QCOMPARE(changes.at(0), KiriView::DocumentSessionChange::MediaNavigationAvailability);
    QCOMPARE(changes.at(1), KiriView::DocumentSessionChange::ActiveNavigation);

    state.setMediaNavigationState(boundary, true);
    QCOMPARE(changes.size(), std::size_t(2));

    boundary.canOpenNext = true;
    state.setMediaNavigationState(boundary, true);
    QCOMPARE(changes.size(), std::size_t(4));
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
