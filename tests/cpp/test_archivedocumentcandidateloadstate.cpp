// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archive/archivedocumentcandidateloadstate.h"

#include <QObject>
#include <QTest>
#include <optional>
#include <vector>

class TestArchiveDocumentCandidateLoadState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pendingLoadsShareOneStartableBatch();
    void loadAddedDuringBatchJoinsActiveBatch();
    void cancelRejectsActiveBatch();
    void wrongBatchCannotFinishPendingLoads();
    void cancelInvalidatesPendingLoads();
};

void TestArchiveDocumentCandidateLoadState::pendingLoadsShareOneStartableBatch()
{
    KiriView::ArchiveDocumentCandidateLoadState state;
    KiriView::ImageIoJob firstJob = state.addLoad(this, {}, {});
    KiriView::ImageIoJob secondJob = state.addLoad(this, {}, {});

    QVERIFY(firstJob.isActive());
    QVERIFY(secondJob.isActive());
    const std::optional<KiriView::ArchiveDocumentCandidateLoadBatch> batch = state.startBatch();
    QVERIFY(batch.has_value());
    QVERIFY(state.batchInProgress());
    QVERIFY(!state.startBatch().has_value());

    std::vector<KiriView::ArchiveDocumentCandidateLoad> loads = state.finishBatch(*batch);
    QCOMPARE(loads.size(), std::size_t(2));
    QVERIFY(loads[0].completion.claimAndDelete([]() { }));
    QVERIFY(loads[1].completion.claimAndDelete([]() { }));
    QVERIFY(!firstJob.isActive());
    QVERIFY(!secondJob.isActive());
    QVERIFY(!state.batchInProgress());
    QVERIFY(!state.startBatch().has_value());
}

void TestArchiveDocumentCandidateLoadState::loadAddedDuringBatchJoinsActiveBatch()
{
    KiriView::ArchiveDocumentCandidateLoadState state;
    KiriView::ImageIoJob firstJob = state.addLoad(this, {}, {});

    const std::optional<KiriView::ArchiveDocumentCandidateLoadBatch> batch = state.startBatch();
    QVERIFY(batch.has_value());
    QVERIFY(state.batchInProgress());
    KiriView::ImageIoJob secondJob = state.addLoad(this, {}, {});

    QVERIFY(firstJob.isActive());
    QVERIFY(secondJob.isActive());
    QVERIFY(!state.startBatch().has_value());

    std::vector<KiriView::ArchiveDocumentCandidateLoad> loads = state.finishBatch(*batch);
    QCOMPARE(loads.size(), std::size_t(2));
    QVERIFY(loads[0].completion.claimAndDelete([]() { }));
    QVERIFY(loads[1].completion.claimAndDelete([]() { }));
    QVERIFY(!firstJob.isActive());
    QVERIFY(!secondJob.isActive());
    QVERIFY(!state.batchInProgress());
}

void TestArchiveDocumentCandidateLoadState::cancelRejectsActiveBatch()
{
    KiriView::ArchiveDocumentCandidateLoadState state;
    KiriView::ImageIoJob job = state.addLoad(this, {}, {});

    const std::optional<KiriView::ArchiveDocumentCandidateLoadBatch> batch = state.startBatch();
    QVERIFY(batch.has_value());
    state.cancel();

    QVERIFY(!job.isActive());
    QCOMPARE(state.finishBatch(*batch).size(), std::size_t(0));
    QVERIFY(!state.acceptsBatch(*batch));
    QVERIFY(!state.batchInProgress());
}

void TestArchiveDocumentCandidateLoadState::wrongBatchCannotFinishPendingLoads()
{
    KiriView::ArchiveDocumentCandidateLoadState state;
    KiriView::ImageIoJob staleJob = state.addLoad(this, {}, {});
    const std::optional<KiriView::ArchiveDocumentCandidateLoadBatch> staleBatch
        = state.startBatch();
    QVERIFY(staleBatch.has_value());
    state.cancel();

    KiriView::ImageIoJob currentJob = state.addLoad(this, {}, {});
    const std::optional<KiriView::ArchiveDocumentCandidateLoadBatch> currentBatch
        = state.startBatch();
    QVERIFY(currentBatch.has_value());
    QVERIFY(!staleJob.isActive());
    QVERIFY(currentJob.isActive());
    QCOMPARE(state.finishBatch(*staleBatch).size(), std::size_t(0));
    QVERIFY(currentJob.isActive());

    std::vector<KiriView::ArchiveDocumentCandidateLoad> loads = state.finishBatch(*currentBatch);
    QCOMPARE(loads.size(), std::size_t(1));
    QVERIFY(loads.front().completion.claimAndDelete([]() { }));
    QVERIFY(!currentJob.isActive());
}

void TestArchiveDocumentCandidateLoadState::cancelInvalidatesPendingLoads()
{
    KiriView::ArchiveDocumentCandidateLoadState state;
    KiriView::ImageIoJob job = state.addLoad(this, {}, {});

    state.cancel();

    QVERIFY(!job.isActive());
    QVERIFY(!state.startBatch().has_value());
    QCOMPARE(state.finishBatch({ 1 }).size(), std::size_t(0));
}

QTEST_GUILESS_MAIN(TestArchiveDocumentCandidateLoadState)

#include "test_archivedocumentcandidateloadstate.moc"
