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
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImageDocumentChangeBatcher batcher(
        [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });

    batcher.notify(kiriview::ImageDocumentChange::Loading);
    batcher.notifyAll(
        { kiriview::ImageDocumentChange::Status, kiriview::ImageDocumentChange::DisplaySource });

    QCOMPARE(changes.size(), std::size_t(3));
    QCOMPARE(changes.at(0), kiriview::ImageDocumentChange::Loading);
    QCOMPARE(changes.at(1), kiriview::ImageDocumentChange::Status);
    QCOMPARE(changes.at(2), kiriview::ImageDocumentChange::DisplaySource);
}

void TestImageDocumentChangeBatcher::batchCallbacksReceiveWholeOrderedBatches()
{
    std::vector<std::vector<kiriview::ImageDocumentChange>> publishedBatches;
    kiriview::ImageDocumentChangeBatcher batcher(
        kiriview::ImageDocumentChangeBatcher::ChangeBatchCallback(
            [&publishedBatches](const std::vector<kiriview::ImageDocumentChange> &changes) {
                publishedBatches.push_back(changes);
            }));

    batcher.notify(kiriview::ImageDocumentChange::Loading);
    {
        [[maybe_unused]] auto batch = batcher.beginBatch();
        batcher.notify(kiriview::ImageDocumentChange::Status);
        batcher.notify(kiriview::ImageDocumentChange::Status);
        batcher.notify(kiriview::ImageDocumentChange::DisplaySource);
        QVERIFY(publishedBatches.size() == std::size_t(1));
    }

    QCOMPARE(publishedBatches.size(), std::size_t(2));
    QCOMPARE(publishedBatches.at(0).size(), std::size_t(1));
    QCOMPARE(publishedBatches.at(0).at(0), kiriview::ImageDocumentChange::Loading);
    QCOMPARE(publishedBatches.at(1).size(), std::size_t(2));
    QCOMPARE(publishedBatches.at(1).at(0), kiriview::ImageDocumentChange::Status);
    QCOMPARE(publishedBatches.at(1).at(1), kiriview::ImageDocumentChange::DisplaySource);
}

void TestImageDocumentChangeBatcher::emptyBatchesDoNotPublish()
{
    int publishCount = 0;
    kiriview::ImageDocumentChangeBatcher batcher(
        kiriview::ImageDocumentChangeBatcher::ChangeBatchCallback(
            [&publishCount](
                const std::vector<kiriview::ImageDocumentChange> &) { ++publishCount; }));

    {
        [[maybe_unused]] auto batch = batcher.beginBatch();
    }

    QCOMPARE(publishCount, 0);
}

void TestImageDocumentChangeBatcher::batchesPublishUniqueChangesWhenOutermostBatchEnds()
{
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImageDocumentChangeBatcher batcher(
        [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });

    {
        [[maybe_unused]] auto batch = batcher.beginBatch();
        batcher.notify(kiriview::ImageDocumentChange::Loading);
        batcher.notify(kiriview::ImageDocumentChange::Loading);
        {
            [[maybe_unused]] auto nestedBatch = batcher.beginBatch();
            batcher.notify(kiriview::ImageDocumentChange::Status);
        }
        QVERIFY(changes.empty());
        batcher.notify(kiriview::ImageDocumentChange::DisplaySource);
    }

    QCOMPARE(changes.size(), std::size_t(3));
    QCOMPARE(changes.at(0), kiriview::ImageDocumentChange::Loading);
    QCOMPARE(changes.at(1), kiriview::ImageDocumentChange::Status);
    QCOMPARE(changes.at(2), kiriview::ImageDocumentChange::DisplaySource);
}

void TestImageDocumentChangeBatcher::movedBatchKeepsSingleFlushOwner()
{
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImageDocumentChangeBatcher batcher(
        [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });

    {
        auto batch = batcher.beginBatch();
        batcher.notify(kiriview::ImageDocumentChange::Loading);
        [[maybe_unused]] auto movedBatch = std::move(batch);
        batcher.notify(kiriview::ImageDocumentChange::Status);
        QVERIFY(changes.empty());
    }

    QCOMPARE(changes.size(), std::size_t(2));
    QCOMPARE(changes.at(0), kiriview::ImageDocumentChange::Loading);
    QCOMPARE(changes.at(1), kiriview::ImageDocumentChange::Status);
}

QTEST_GUILESS_MAIN(TestImageDocumentChangeBatcher)

#include "test_imagedocumentchangebatcher.moc"
