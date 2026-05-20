// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archive/archivedocumentcandidateloadstate.h"

#include <QObject>
#include <QTest>
#include <vector>

class TestArchiveDocumentCandidateLoadState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sameGenerationLoadsShareOneStartableBatch();
    void generationChangeCancelsPreviousBatch();
    void wrongGenerationCannotFinishBatch();
    void cancelInvalidatesPendingLoads();
};

void TestArchiveDocumentCandidateLoadState::sameGenerationLoadsShareOneStartableBatch()
{
    KiriView::ArchiveDocumentCandidateLoadState state;
    KiriView::ImageIoJob firstJob = state.addLoad(this, 7, {}, {});
    KiriView::ImageIoJob secondJob = state.addLoad(this, 7, {}, {});

    QVERIFY(firstJob.isActive());
    QVERIFY(secondJob.isActive());
    QVERIFY(state.startBatch(7));
    QVERIFY(!state.startBatch(7));

    std::vector<KiriView::ArchiveDocumentCandidateLoad> loads = state.finishBatch(7);
    QCOMPARE(loads.size(), std::size_t(2));
    QVERIFY(loads[0].completion.claimAndDelete([]() { }));
    QVERIFY(loads[1].completion.claimAndDelete([]() { }));
    QVERIFY(!firstJob.isActive());
    QVERIFY(!secondJob.isActive());
    QVERIFY(!state.startBatch(7));
}

void TestArchiveDocumentCandidateLoadState::generationChangeCancelsPreviousBatch()
{
    KiriView::ArchiveDocumentCandidateLoadState state;
    KiriView::ImageIoJob staleJob = state.addLoad(this, 3, {}, {});

    QVERIFY(state.startBatch(3));
    KiriView::ImageIoJob currentJob = state.addLoad(this, 4, {}, {});

    QVERIFY(!staleJob.isActive());
    QVERIFY(currentJob.isActive());
    QVERIFY(!state.startBatch(3));
    QVERIFY(state.startBatch(4));
    QCOMPARE(state.finishBatch(3).size(), std::size_t(0));
    QCOMPARE(state.finishBatch(4).size(), std::size_t(1));
}

void TestArchiveDocumentCandidateLoadState::wrongGenerationCannotFinishBatch()
{
    KiriView::ArchiveDocumentCandidateLoadState state;
    KiriView::ImageIoJob job = state.addLoad(this, 9, {}, {});

    QVERIFY(state.startBatch(9));
    QCOMPARE(state.finishBatch(10).size(), std::size_t(0));
    QVERIFY(job.isActive());

    std::vector<KiriView::ArchiveDocumentCandidateLoad> loads = state.finishBatch(9);
    QCOMPARE(loads.size(), std::size_t(1));
    QVERIFY(loads.front().completion.claimAndDelete([]() { }));
    QVERIFY(!job.isActive());
}

void TestArchiveDocumentCandidateLoadState::cancelInvalidatesPendingLoads()
{
    KiriView::ArchiveDocumentCandidateLoadState state;
    KiriView::ImageIoJob job = state.addLoad(this, 11, {}, {});

    state.cancel();

    QVERIFY(!job.isActive());
    QVERIFY(!state.startBatch(11));
    QCOMPARE(state.finishBatch(11).size(), std::size_t(0));
}

QTEST_GUILESS_MAIN(TestArchiveDocumentCandidateLoadState)

#include "test_archivedocumentcandidateloadstate.moc"
