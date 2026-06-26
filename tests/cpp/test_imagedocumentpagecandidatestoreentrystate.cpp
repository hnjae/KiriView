// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagedocumentpagecandidatestoreentrystate.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <utility>
#include <vector>

namespace {
QUrl localUrl(const QString& path) { return QUrl::fromLocalFile(path); }

kiriview::ImageDocumentPageCandidate candidate(const QString& path, const QString& name)
{
    return kiriview::ImageDocumentPageCandidate {
        localUrl(path),
        name,
    };
}

std::vector<kiriview::ImageDocumentPageCandidate> candidates(std::initializer_list<QString> names)
{
    std::vector<kiriview::ImageDocumentPageCandidate> result;
    result.reserve(names.size());
    for (const QString& name : names) {
        result.push_back(candidate(QStringLiteral("/images/%1").arg(name), name));
    }
    return result;
}

kiriview::ImageIoJob addPendingLoad(kiriview::ImageDocumentPageCandidateStoreEntryState& state,
    QObject* parent, kiriview::ImageDocumentPageCandidatesCallback callback,
    kiriview::ErrorCallback errorCallback, std::function<void(QObject*)> removeToken)
{
    QObject* token = new QObject(parent);
    kiriview::ImageIoJob job(token, [removeToken = std::move(removeToken)](QObject* object) {
        removeToken(object);
        object->deleteLater();
    });
    state.addPendingLoad(job.completion(), std::move(callback), std::move(errorCallback));
    return job;
}

kiriview::ImageIoJob addSubscriber(kiriview::ImageDocumentPageCandidateStoreEntryState& state,
    QObject* parent, kiriview::ImageDocumentPageCandidatesCallback callback,
    kiriview::ErrorCallback errorCallback, std::function<void(QObject*)> removeToken)
{
    QObject* token = new QObject(parent);
    kiriview::ImageIoJob job(token, [removeToken = std::move(removeToken)](QObject* object) {
        removeToken(object);
        object->deleteLater();
    });
    state.addSubscriber(token, std::move(callback), std::move(errorCallback));
    return job;
}

void deliverCandidatePlan(
    const kiriview::ImageDocumentPageCandidateStoreEntryNotificationPlan& plan)
{
    for (const kiriview::ImageDocumentPageCandidateStoreEntryPendingLoad& load :
        plan.completedLoads) {
        load.completion.claimAndDelete([&]() {
            if (load.callback) {
                load.callback(plan.candidates);
            }
        });
    }
    for (const kiriview::ImageDocumentPageCandidateStoreEntrySubscriber& subscriber :
        plan.changedSubscribers) {
        if (subscriber.callback) {
            subscriber.callback(plan.candidates);
        }
    }
}

void deliverErrorPlan(const kiriview::ImageDocumentPageCandidateStoreEntryNotificationPlan& plan)
{
    for (const kiriview::ImageDocumentPageCandidateStoreEntryPendingLoad& load : plan.failedLoads) {
        load.completion.claimAndDelete([&]() {
            if (load.errorCallback) {
                load.errorCallback(plan.errorString);
            }
        });
    }
    for (const kiriview::ImageDocumentPageCandidateStoreEntrySubscriber& subscriber :
        plan.failedSubscribers) {
        if (subscriber.errorCallback) {
            subscriber.errorCallback(plan.errorString);
        }
    }
}
}

class TestImageDocumentPageCandidateStoreEntryState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void completedListingReturnsPendingLoadPlanAndCachesCandidates();
    void cancelledPendingLoadDoesNotReceiveCompletion();
    void subscribersOnlyReceivePlansAfterInitialListing();
    void failedListingReturnsErrorPlansForPendingLoadsAndSubscribers();
};

void TestImageDocumentPageCandidateStoreEntryState::
    completedListingReturnsPendingLoadPlanAndCachesCandidates()
{
    kiriview::ImageDocumentPageCandidateStoreEntryState state;
    std::vector<kiriview::ImageDocumentPageCandidate> loadedCandidates;
    int errorCount = 0;
    int cancelCount = 0;

    kiriview::ImageIoJob job = addPendingLoad(
        state, this,
        [&loadedCandidates](std::vector<kiriview::ImageDocumentPageCandidate> candidates) {
            loadedCandidates = std::move(candidates);
        },
        [&errorCount](const QString&) { ++errorCount; },
        [&state, &cancelCount](QObject* token) {
            ++cancelCount;
            state.removePendingLoad(token);
        });

    QVERIFY(job.isActive());
    const kiriview::ImageDocumentPageCandidateStoreEntryNotificationPlan plan
        = state.completeListing(candidates({ QStringLiteral("01.png"), QStringLiteral("02.png") }));

    QVERIFY(state.listed());
    QVERIFY(!state.failed());
    QCOMPARE(plan.completedLoads.size(), std::size_t(1));
    QCOMPARE(plan.failedLoads.size(), std::size_t(0));
    QCOMPARE(plan.changedSubscribers.size(), std::size_t(0));
    QCOMPARE(plan.candidates.size(), std::size_t(2));
    QVERIFY(job.isActive());
    deliverCandidatePlan(plan);
    QVERIFY(!job.isActive());
    QCOMPARE(errorCount, 0);
    QCOMPARE(cancelCount, 0);
    QCOMPARE(loadedCandidates.size(), std::size_t(2));
    QCOMPARE(loadedCandidates.size(), state.candidates().size());
    QCOMPARE(loadedCandidates.front().url, state.candidates().front().url);
    QCOMPARE(loadedCandidates.front().name, state.candidates().front().name);
    QCOMPARE(loadedCandidates.back().url, state.candidates().back().url);
    QCOMPARE(loadedCandidates.back().name, state.candidates().back().name);
}

void TestImageDocumentPageCandidateStoreEntryState::cancelledPendingLoadDoesNotReceiveCompletion()
{
    kiriview::ImageDocumentPageCandidateStoreEntryState state;
    int loadCount = 0;
    int cancelCount = 0;

    kiriview::ImageIoJob job = addPendingLoad(
        state, this,
        [&loadCount](std::vector<kiriview::ImageDocumentPageCandidate>) { ++loadCount; },
        [](const QString&) {},
        [&state, &cancelCount](QObject* token) {
            ++cancelCount;
            state.removePendingLoad(token);
        });

    job.cancel();
    const kiriview::ImageDocumentPageCandidateStoreEntryNotificationPlan plan
        = state.completeListing(candidates({ QStringLiteral("01.png") }));

    QCOMPARE(plan.completedLoads.size(), std::size_t(0));
    QCOMPARE(cancelCount, 1);
    QCOMPARE(loadCount, 0);
    QVERIFY(!job.isActive());
}

void TestImageDocumentPageCandidateStoreEntryState::subscribersOnlyReceivePlansAfterInitialListing()
{
    kiriview::ImageDocumentPageCandidateStoreEntryState state;
    std::vector<kiriview::ImageDocumentPageCandidate> changedCandidates;
    int changeCount = 0;
    int cancelCount = 0;

    kiriview::ImageIoJob job = addSubscriber(
        state, this,
        [&changedCandidates, &changeCount](
            std::vector<kiriview::ImageDocumentPageCandidate> candidates) {
            changedCandidates = std::move(candidates);
            ++changeCount;
        },
        [](const QString&) {},
        [&state, &cancelCount](QObject* token) {
            ++cancelCount;
            state.removeSubscriber(token);
        });

    kiriview::ImageDocumentPageCandidateStoreEntryNotificationPlan plan
        = state.completeListing(candidates({ QStringLiteral("01.png") }));
    QCOMPARE(plan.changedSubscribers.size(), std::size_t(0));
    QCOMPARE(changeCount, 0);

    plan = state.updateListing(candidates({ QStringLiteral("01.png") }));
    QCOMPARE(plan.changedSubscribers.size(), std::size_t(0));
    QCOMPARE(changeCount, 0);

    plan = state.updateListing(candidates({ QStringLiteral("01.png"), QStringLiteral("02.png") }));
    QCOMPARE(plan.changedSubscribers.size(), std::size_t(1));
    QCOMPARE(plan.candidates.size(), std::size_t(2));
    deliverCandidatePlan(plan);
    QCOMPARE(changeCount, 1);
    QCOMPARE(changedCandidates.size(), std::size_t(2));

    job.cancel();
    plan = state.updateListing(candidates(
        { QStringLiteral("01.png"), QStringLiteral("02.png"), QStringLiteral("03.png") }));
    QCOMPARE(plan.changedSubscribers.size(), std::size_t(0));
    QCOMPARE(cancelCount, 1);
    QCOMPARE(changeCount, 1);
}

void TestImageDocumentPageCandidateStoreEntryState::
    failedListingReturnsErrorPlansForPendingLoadsAndSubscribers()
{
    kiriview::ImageDocumentPageCandidateStoreEntryState state;
    QString pendingError;
    QString subscriberError;
    int loadCount = 0;
    int subscriberChangeCount = 0;

    kiriview::ImageIoJob pending = addPendingLoad(
        state, this,
        [&loadCount](std::vector<kiriview::ImageDocumentPageCandidate>) { ++loadCount; },
        [&pendingError](const QString& errorString) { pendingError = errorString; },
        [&state](QObject* token) { state.removePendingLoad(token); });
    kiriview::ImageIoJob subscriber = addSubscriber(
        state, this,
        [&subscriberChangeCount](
            std::vector<kiriview::ImageDocumentPageCandidate>) { ++subscriberChangeCount; },
        [&subscriberError](const QString& errorString) { subscriberError = errorString; },
        [&state](QObject* token) { state.removeSubscriber(token); });

    const kiriview::ImageDocumentPageCandidateStoreEntryNotificationPlan plan
        = state.failListing(QStringLiteral("missing directory"));

    QVERIFY(state.failed());
    QCOMPARE(state.errorString(), QStringLiteral("missing directory"));
    QCOMPARE(plan.failedLoads.size(), std::size_t(1));
    QCOMPARE(plan.failedSubscribers.size(), std::size_t(1));
    QCOMPARE(plan.errorString, QStringLiteral("missing directory"));
    QVERIFY(pending.isActive());
    QVERIFY(subscriber.isActive());
    deliverErrorPlan(plan);
    QVERIFY(!pending.isActive());
    QVERIFY(subscriber.isActive());
    QCOMPARE(loadCount, 0);
    QCOMPARE(subscriberChangeCount, 0);
    QCOMPARE(pendingError, QStringLiteral("missing directory"));
    QCOMPARE(subscriberError, QStringLiteral("missing directory"));
}

QTEST_GUILESS_MAIN(TestImageDocumentPageCandidateStoreEntryState)

#include "test_imagedocumentpagecandidatestoreentrystate.moc"
