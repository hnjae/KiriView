// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageiojob.h"

#include <QObject>
#include <QTest>
#include <memory>
#include <utility>

class TestImageIoJob : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cancelInvokesCallbackOnce();
    void claimCompletesJobWithoutCanceling();
    void destroyedObjectDeactivatesJobWithoutCanceling();
    void moveAssignmentCancelsReplacedJob();
};

void TestImageIoJob::cancelInvokesCallbackOnce()
{
    QObject object;
    int cancelCount = 0;
    QObject *canceledObject = nullptr;
    KiriView::ImageIoJob job(&object, [&cancelCount, &canceledObject](QObject *objectToCancel) {
        ++cancelCount;
        canceledObject = objectToCancel;
    });

    QVERIFY(job.isActive());
    job.cancel();
    job.cancel();

    QCOMPARE(cancelCount, 1);
    QCOMPARE(canceledObject, &object);
    QVERIFY(!job.isActive());
    QVERIFY(!job.state()->claim(&object));
}

void TestImageIoJob::claimCompletesJobWithoutCanceling()
{
    QObject object;
    int cancelCount = 0;
    KiriView::ImageIoJob job(&object, [&cancelCount](QObject *) { ++cancelCount; });
    const std::shared_ptr<KiriView::ImageIoJobState> jobState = job.state();

    QVERIFY(jobState->claim(&object));
    QVERIFY(!job.isActive());
    job.cancel();

    QCOMPARE(cancelCount, 0);
}

void TestImageIoJob::destroyedObjectDeactivatesJobWithoutCanceling()
{
    int cancelCount = 0;
    KiriView::ImageIoJob job;

    {
        auto object = std::make_unique<QObject>();
        job = KiriView::ImageIoJob(object.get(), [&cancelCount](QObject *) { ++cancelCount; });
        QVERIFY(job.isActive());
    }

    QVERIFY(!job.isActive());
    job.cancel();

    QCOMPARE(cancelCount, 0);
}

void TestImageIoJob::moveAssignmentCancelsReplacedJob()
{
    QObject firstObject;
    QObject secondObject;
    int firstCancelCount = 0;
    int secondCancelCount = 0;
    KiriView::ImageIoJob job(&firstObject, [&firstCancelCount](QObject *) { ++firstCancelCount; });
    KiriView::ImageIoJob replacement(
        &secondObject, [&secondCancelCount](QObject *) { ++secondCancelCount; });

    job = std::move(replacement);

    QCOMPARE(firstCancelCount, 1);
    QCOMPARE(secondCancelCount, 0);
    QVERIFY(job.isActive());

    job.cancel();
    QCOMPARE(secondCancelCount, 1);
}

QTEST_GUILESS_MAIN(TestImageIoJob)

#include "test_imageiojob.moc"
