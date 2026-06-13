// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archive/mediaentrysourcecandidateloadstate.h"

#include <QObject>
#include <QTest>
#include <optional>
#include <vector>

class TestMediaEntrySourceCandidateLoadState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pendingLoadsShareOneStartableBatch();
    void loadAddedDuringBatchJoinsActiveBatch();
    void cancelRejectsActiveBatch();
    void wrongBatchCannotFinishPendingLoads();
    void cancelInvalidatesPendingLoads();
};

void TestMediaEntrySourceCandidateLoadState::pendingLoadsShareOneStartableBatch()
{
    kiriview::MediaEntrySourceCandidateLoadState state;
    kiriview::ImageIoJob firstJob = state.addLoad(this, {}, {});
    kiriview::ImageIoJob secondJob = state.addLoad(this, {}, {});

    QVERIFY(firstJob.isActive());
    QVERIFY(secondJob.isActive());
    const std::optional<kiriview::MediaEntrySourceCandidateLoadBatch> batch = state.startBatch();
    QVERIFY(batch.has_value());
    QVERIFY(state.batchInProgress());
    QVERIFY(!state.startBatch().has_value());

    std::vector<kiriview::MediaEntrySourceCandidateLoad> loads = state.finishBatch(*batch);
    QCOMPARE(loads.size(), std::size_t(2));
    QVERIFY(loads[0].completion.claimAndDelete([]() { }));
    QVERIFY(loads[1].completion.claimAndDelete([]() { }));
    QVERIFY(!firstJob.isActive());
    QVERIFY(!secondJob.isActive());
    QVERIFY(!state.batchInProgress());
    QVERIFY(!state.startBatch().has_value());
}

void TestMediaEntrySourceCandidateLoadState::loadAddedDuringBatchJoinsActiveBatch()
{
    kiriview::MediaEntrySourceCandidateLoadState state;
    kiriview::ImageIoJob firstJob = state.addLoad(this, {}, {});

    const std::optional<kiriview::MediaEntrySourceCandidateLoadBatch> batch = state.startBatch();
    QVERIFY(batch.has_value());
    QVERIFY(state.batchInProgress());
    kiriview::ImageIoJob secondJob = state.addLoad(this, {}, {});

    QVERIFY(firstJob.isActive());
    QVERIFY(secondJob.isActive());
    QVERIFY(!state.startBatch().has_value());

    std::vector<kiriview::MediaEntrySourceCandidateLoad> loads = state.finishBatch(*batch);
    QCOMPARE(loads.size(), std::size_t(2));
    QVERIFY(loads[0].completion.claimAndDelete([]() { }));
    QVERIFY(loads[1].completion.claimAndDelete([]() { }));
    QVERIFY(!firstJob.isActive());
    QVERIFY(!secondJob.isActive());
    QVERIFY(!state.batchInProgress());
}

void TestMediaEntrySourceCandidateLoadState::cancelRejectsActiveBatch()
{
    kiriview::MediaEntrySourceCandidateLoadState state;
    kiriview::ImageIoJob job = state.addLoad(this, {}, {});

    const std::optional<kiriview::MediaEntrySourceCandidateLoadBatch> batch = state.startBatch();
    QVERIFY(batch.has_value());
    state.cancel();

    QVERIFY(!job.isActive());
    QCOMPARE(state.finishBatch(*batch).size(), std::size_t(0));
    QVERIFY(!state.acceptsBatch(*batch));
    QVERIFY(!state.batchInProgress());
}

void TestMediaEntrySourceCandidateLoadState::wrongBatchCannotFinishPendingLoads()
{
    kiriview::MediaEntrySourceCandidateLoadState state;
    kiriview::ImageIoJob staleJob = state.addLoad(this, {}, {});
    const std::optional<kiriview::MediaEntrySourceCandidateLoadBatch> staleBatch
        = state.startBatch();
    QVERIFY(staleBatch.has_value());
    state.cancel();

    kiriview::ImageIoJob currentJob = state.addLoad(this, {}, {});
    const std::optional<kiriview::MediaEntrySourceCandidateLoadBatch> currentBatch
        = state.startBatch();
    QVERIFY(currentBatch.has_value());
    QVERIFY(!staleJob.isActive());
    QVERIFY(currentJob.isActive());
    QCOMPARE(state.finishBatch(*staleBatch).size(), std::size_t(0));
    QVERIFY(currentJob.isActive());

    std::vector<kiriview::MediaEntrySourceCandidateLoad> loads = state.finishBatch(*currentBatch);
    QCOMPARE(loads.size(), std::size_t(1));
    QVERIFY(loads.front().completion.claimAndDelete([]() { }));
    QVERIFY(!currentJob.isActive());
}

void TestMediaEntrySourceCandidateLoadState::cancelInvalidatesPendingLoads()
{
    kiriview::MediaEntrySourceCandidateLoadState state;
    kiriview::ImageIoJob job = state.addLoad(this, {}, {});

    state.cancel();

    QVERIFY(!job.isActive());
    QVERIFY(!state.startBatch().has_value());
    QCOMPARE(state.finishBatch({ 1 }).size(), std::size_t(0));
}

QTEST_GUILESS_MAIN(TestMediaEntrySourceCandidateLoadState)

#include "test_mediaentrysourcecandidateloadstate.moc"
