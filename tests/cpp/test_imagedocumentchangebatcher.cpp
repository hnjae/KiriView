// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentchangebatcher.h"

#include <QObject>
#include <QTest>
#include <cstddef>
#include <utility>
#include <vector>

class TestImageDocumentChangeBatcher : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void immediateNotificationsForwardInOrder();
    void batchCallbacksReceiveWholeOrderedBatches();
    void emptyBatchesDoNotPublish();
    void batchesPublishUniqueChangesWhenOutermostBatchEnds();
    void movedBatchKeepsSingleFlushOwner();
};

void TestImageDocumentChangeBatcher::immediateNotificationsForwardInOrder()
{
    std::vector<KiriView::ImageDocumentChange> changes;
    KiriView::ImageDocumentChangeBatcher batcher(
        [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });

    batcher.notify(KiriView::ImageDocumentChange::Loading);
    batcher.notifyAll(
        { KiriView::ImageDocumentChange::Status, KiriView::ImageDocumentChange::Repaint });

    QCOMPARE(changes.size(), std::size_t(3));
    QCOMPARE(changes.at(0), KiriView::ImageDocumentChange::Loading);
    QCOMPARE(changes.at(1), KiriView::ImageDocumentChange::Status);
    QCOMPARE(changes.at(2), KiriView::ImageDocumentChange::Repaint);
}

void TestImageDocumentChangeBatcher::batchCallbacksReceiveWholeOrderedBatches()
{
    std::vector<std::vector<KiriView::ImageDocumentChange>> publishedBatches;
    KiriView::ImageDocumentChangeBatcher batcher(
        KiriView::ImageDocumentChangeBatcher::ChangeBatchCallback(
            [&publishedBatches](const std::vector<KiriView::ImageDocumentChange> &changes) {
                publishedBatches.push_back(changes);
            }));

    batcher.notify(KiriView::ImageDocumentChange::Loading);
    {
        [[maybe_unused]] auto batch = batcher.beginBatch();
        batcher.notify(KiriView::ImageDocumentChange::Status);
        batcher.notify(KiriView::ImageDocumentChange::Status);
        batcher.notify(KiriView::ImageDocumentChange::Repaint);
        QVERIFY(publishedBatches.size() == std::size_t(1));
    }

    QCOMPARE(publishedBatches.size(), std::size_t(2));
    QCOMPARE(publishedBatches.at(0).size(), std::size_t(1));
    QCOMPARE(publishedBatches.at(0).at(0), KiriView::ImageDocumentChange::Loading);
    QCOMPARE(publishedBatches.at(1).size(), std::size_t(2));
    QCOMPARE(publishedBatches.at(1).at(0), KiriView::ImageDocumentChange::Status);
    QCOMPARE(publishedBatches.at(1).at(1), KiriView::ImageDocumentChange::Repaint);
}

void TestImageDocumentChangeBatcher::emptyBatchesDoNotPublish()
{
    int publishCount = 0;
    KiriView::ImageDocumentChangeBatcher batcher(
        KiriView::ImageDocumentChangeBatcher::ChangeBatchCallback(
            [&publishCount](
                const std::vector<KiriView::ImageDocumentChange> &) { ++publishCount; }));

    {
        [[maybe_unused]] auto batch = batcher.beginBatch();
    }

    QCOMPARE(publishCount, 0);
}

void TestImageDocumentChangeBatcher::batchesPublishUniqueChangesWhenOutermostBatchEnds()
{
    std::vector<KiriView::ImageDocumentChange> changes;
    KiriView::ImageDocumentChangeBatcher batcher(
        [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });

    {
        [[maybe_unused]] auto batch = batcher.beginBatch();
        batcher.notify(KiriView::ImageDocumentChange::Loading);
        batcher.notify(KiriView::ImageDocumentChange::Loading);
        {
            [[maybe_unused]] auto nestedBatch = batcher.beginBatch();
            batcher.notify(KiriView::ImageDocumentChange::Status);
        }
        QVERIFY(changes.empty());
        batcher.notify(KiriView::ImageDocumentChange::Repaint);
    }

    QCOMPARE(changes.size(), std::size_t(3));
    QCOMPARE(changes.at(0), KiriView::ImageDocumentChange::Loading);
    QCOMPARE(changes.at(1), KiriView::ImageDocumentChange::Status);
    QCOMPARE(changes.at(2), KiriView::ImageDocumentChange::Repaint);
}

void TestImageDocumentChangeBatcher::movedBatchKeepsSingleFlushOwner()
{
    std::vector<KiriView::ImageDocumentChange> changes;
    KiriView::ImageDocumentChangeBatcher batcher(
        [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });

    {
        auto batch = batcher.beginBatch();
        batcher.notify(KiriView::ImageDocumentChange::Loading);
        [[maybe_unused]] auto movedBatch = std::move(batch);
        batcher.notify(KiriView::ImageDocumentChange::Status);
        QVERIFY(changes.empty());
    }

    QCOMPARE(changes.size(), std::size_t(2));
    QCOMPARE(changes.at(0), KiriView::ImageDocumentChange::Loading);
    QCOMPARE(changes.at(1), KiriView::ImageDocumentChange::Status);
}

QTEST_GUILESS_MAIN(TestImageDocumentChangeBatcher)

#include "test_imagedocumentchangebatcher.moc"
