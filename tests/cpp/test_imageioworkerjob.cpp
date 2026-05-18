// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageioworkerjob.h"

#include <QSemaphore>
#include <QTest>
#include <atomic>

class TestImageIoWorkerJob : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nullReceiverRunsSynchronously();
    void workerCompletionFinishesJobOnce();
    void canceledWorkerCompletionIsIgnored();
};

void TestImageIoWorkerJob::nullReceiverRunsSynchronously()
{
    int workCount = 0;
    int finishValue = 0;

    KiriView::ImageIoJob job = KiriView::startImageIoWorkerJob(
        nullptr,
        [&workCount]() {
            ++workCount;
            return 7;
        },
        [&finishValue](int value) { finishValue = value; });

    QVERIFY(!job.isActive());
    QCOMPARE(workCount, 1);
    QCOMPARE(finishValue, 7);
}

void TestImageIoWorkerJob::workerCompletionFinishesJobOnce()
{
    QObject receiver;
    int finishCount = 0;
    int finishValue = 0;

    KiriView::ImageIoJob job = KiriView::startImageIoWorkerJob(
        &receiver, []() { return 11; },
        [&finishCount, &finishValue](int value) {
            ++finishCount;
            finishValue = value;
        });

    QTRY_COMPARE(finishCount, 1);
    QCOMPARE(finishValue, 11);
    QVERIFY(!job.isActive());
}

void TestImageIoWorkerJob::canceledWorkerCompletionIsIgnored()
{
    QObject context;
    QObject receiver;
    QSemaphore releaseWorker;
    std::atomic_bool workFinished = false;
    int finishCount = 0;

    KiriView::ImageIoJob job = KiriView::startImageIoWorkerJob(
        &context, &receiver,
        [&releaseWorker, &workFinished]() {
            releaseWorker.acquire();
            workFinished = true;
            return 13;
        },
        [&finishCount](int) { ++finishCount; });

    QVERIFY(job.isActive());
    job.cancel();
    QVERIFY(!job.isActive());

    releaseWorker.release();
    QTRY_VERIFY(workFinished.load());
    QTest::qWait(50);
    QCOMPARE(finishCount, 0);
}

QTEST_GUILESS_MAIN(TestImageIoWorkerJob)

#include "test_imageioworkerjob.moc"
