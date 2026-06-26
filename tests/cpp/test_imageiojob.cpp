// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "async/imageiojob.h"

#include <QCoreApplication>
#include <QEvent>
#include <QObject>
#include <QTest>
#include <memory>
#include <utility>

class TestImageIoJob : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cancelInvokesCallbackOnce();
    void completionClaimCompletesJobWithoutCanceling();
    void completionClaimAndRunCompletesJobOnce();
    void completionClaimAndDeleteCompletesJobOnce();
    void completionTokenRemainsAvailableDuringCancelCallback();
    void destroyedObjectDeactivatesJobWithoutCanceling();
    void moveAssignmentCancelsReplacedJob();
};

void TestImageIoJob::cancelInvokesCallbackOnce()
{
    QObject object;
    int cancelCount = 0;
    QObject* canceledObject = nullptr;
    kiriview::ImageIoJob job(&object, [&cancelCount, &canceledObject](QObject* objectToCancel) {
        ++cancelCount;
        canceledObject = objectToCancel;
    });

    QVERIFY(job.isActive());
    job.cancel();
    job.cancel();

    QCOMPARE(cancelCount, 1);
    QCOMPARE(canceledObject, &object);
    QVERIFY(!job.isActive());
    QVERIFY(!job.completion().claimAndRun([]() { }));
}

void TestImageIoJob::completionClaimCompletesJobWithoutCanceling()
{
    QObject object;
    int cancelCount = 0;
    kiriview::ImageIoJob job(&object, [&cancelCount](QObject*) { ++cancelCount; });
    const kiriview::ImageIoJobCompletion completion = job.completion();

    QVERIFY(completion.claimAndRun([]() { }));
    QVERIFY(!job.isActive());
    job.cancel();

    QCOMPARE(cancelCount, 0);
}

void TestImageIoJob::completionClaimAndRunCompletesJobOnce()
{
    QObject object;
    int finishCount = 0;
    int cancelCount = 0;
    kiriview::ImageIoJob job(&object, [&cancelCount](QObject*) { ++cancelCount; });
    const kiriview::ImageIoJobCompletion completion = job.completion();

    QCOMPARE(completion.object(), &object);
    QVERIFY(completion.isActive());
    QVERIFY(completion.claimAndRun([&finishCount]() { ++finishCount; }));
    QVERIFY(!completion.claimAndRun([&finishCount]() { ++finishCount; }));
    job.cancel();

    QCOMPARE(finishCount, 1);
    QCOMPARE(cancelCount, 0);
    QVERIFY(!job.isActive());
    QVERIFY(!completion.isActive());
}

void TestImageIoJob::completionClaimAndDeleteCompletesJobOnce()
{
    auto* object = new QObject();
    int finishCount = 0;
    int cancelCount = 0;
    kiriview::ImageIoJob job(object, [&cancelCount](QObject*) { ++cancelCount; });
    const kiriview::ImageIoJobCompletion completion = job.completion();

    QVERIFY(completion.claimAndDelete([&finishCount]() { ++finishCount; }));
    QVERIFY(!completion.claimAndDelete([&finishCount]() { ++finishCount; }));
    QCoreApplication::sendPostedEvents(object, QEvent::DeferredDelete);

    QCOMPARE(finishCount, 1);
    QCOMPARE(cancelCount, 0);
    QVERIFY(!job.isActive());
    QVERIFY(completion.object() == nullptr);
}

void TestImageIoJob::completionTokenRemainsAvailableDuringCancelCallback()
{
    QObject object;
    kiriview::ImageIoJobCompletion completion;
    int cancelCount = 0;
    kiriview::ImageIoJob job(&object, [&completion, &object, &cancelCount](QObject* token) {
        ++cancelCount;
        QCOMPARE(token, &object);
        QCOMPARE(completion.object(), &object);
        QVERIFY(!completion.isActive());
    });
    completion = job.completion();

    job.cancel();

    QCOMPARE(cancelCount, 1);
    QCOMPARE(completion.object(), &object);
    QVERIFY(!completion.isActive());
    QVERIFY(!completion.claimAndRun([]() { }));
}

void TestImageIoJob::destroyedObjectDeactivatesJobWithoutCanceling()
{
    int cancelCount = 0;
    kiriview::ImageIoJob job;

    {
        auto object = std::make_unique<QObject>();
        job = kiriview::ImageIoJob(object.get(), [&cancelCount](QObject*) { ++cancelCount; });
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
    kiriview::ImageIoJob job(&firstObject, [&firstCancelCount](QObject*) { ++firstCancelCount; });
    kiriview::ImageIoJob replacement(
        &secondObject, [&secondCancelCount](QObject*) { ++secondCancelCount; });

    job = std::move(replacement);

    QCOMPARE(firstCancelCount, 1);
    QCOMPARE(secondCancelCount, 0);
    QVERIFY(job.isActive());

    job.cancel();
    QCOMPARE(secondCancelCount, 1);
}

QTEST_GUILESS_MAIN(TestImageIoJob)

#include "test_imageiojob.moc"
