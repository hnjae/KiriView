// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagecandidatestoreentrystate.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <initializer_list>
#include <utility>
#include <vector>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

KiriView::ImageNavigationCandidate candidate(const QString &path, const QString &name)
{
    return KiriView::ImageNavigationCandidate {
        localUrl(path),
        name,
    };
}

std::vector<KiriView::ImageNavigationCandidate> candidates(std::initializer_list<QString> names)
{
    std::vector<KiriView::ImageNavigationCandidate> result;
    result.reserve(names.size());
    for (const QString &name : names) {
        result.push_back(candidate(QStringLiteral("/images/%1").arg(name), name));
    }
    return result;
}

void deliverCandidatePlan(const KiriView::ImageCandidateStoreEntryNotificationPlan &plan)
{
    for (const KiriView::ImageCandidateStoreEntryPendingLoad &load : plan.completedLoads) {
        load.completion.claimAndDelete([&]() {
            if (load.callback) {
                load.callback(plan.candidates);
            }
        });
    }
    for (const KiriView::ImageCandidateStoreEntrySubscriber &subscriber : plan.changedSubscribers) {
        if (subscriber.callback) {
            subscriber.callback(plan.candidates);
        }
    }
}

void deliverErrorPlan(const KiriView::ImageCandidateStoreEntryNotificationPlan &plan)
{
    for (const KiriView::ImageCandidateStoreEntryPendingLoad &load : plan.failedLoads) {
        load.completion.claimAndDelete([&]() {
            if (load.errorCallback) {
                load.errorCallback(plan.errorString);
            }
        });
    }
    for (const KiriView::ImageCandidateStoreEntrySubscriber &subscriber : plan.failedSubscribers) {
        if (subscriber.errorCallback) {
            subscriber.errorCallback(plan.errorString);
        }
    }
}
}

class TestImageCandidateStoreEntryState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void completedListingReturnsPendingLoadPlanAndCachesCandidates();
    void cancelledPendingLoadDoesNotReceiveCompletion();
    void subscribersOnlyReceivePlansAfterInitialListing();
    void failedListingReturnsErrorPlansForPendingLoadsAndSubscribers();
};

void TestImageCandidateStoreEntryState::completedListingReturnsPendingLoadPlanAndCachesCandidates()
{
    KiriView::ImageCandidateStoreEntryState state;
    std::vector<KiriView::ImageNavigationCandidate> loadedCandidates;
    int errorCount = 0;
    int cancelCount = 0;

    KiriView::ImageIoJob job = state.addPendingLoad(
        this, this,
        [&loadedCandidates](std::vector<KiriView::ImageNavigationCandidate> candidates) {
            loadedCandidates = std::move(candidates);
        },
        [&errorCount](const QString &) { ++errorCount; },
        [&state, &cancelCount](QObject *token) {
            ++cancelCount;
            state.removePendingLoad(token);
        });

    QVERIFY(job.isActive());
    const KiriView::ImageCandidateStoreEntryNotificationPlan plan
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

void TestImageCandidateStoreEntryState::cancelledPendingLoadDoesNotReceiveCompletion()
{
    KiriView::ImageCandidateStoreEntryState state;
    int loadCount = 0;
    int cancelCount = 0;

    KiriView::ImageIoJob job = state.addPendingLoad(
        this, this, [&loadCount](std::vector<KiriView::ImageNavigationCandidate>) { ++loadCount; },
        [](const QString &) {},
        [&state, &cancelCount](QObject *token) {
            ++cancelCount;
            state.removePendingLoad(token);
        });

    job.cancel();
    const KiriView::ImageCandidateStoreEntryNotificationPlan plan
        = state.completeListing(candidates({ QStringLiteral("01.png") }));

    QCOMPARE(plan.completedLoads.size(), std::size_t(0));
    QCOMPARE(cancelCount, 1);
    QCOMPARE(loadCount, 0);
    QVERIFY(!job.isActive());
}

void TestImageCandidateStoreEntryState::subscribersOnlyReceivePlansAfterInitialListing()
{
    KiriView::ImageCandidateStoreEntryState state;
    std::vector<KiriView::ImageNavigationCandidate> changedCandidates;
    int changeCount = 0;
    int cancelCount = 0;

    KiriView::ImageIoJob job = state.addSubscriber(
        this, this,
        [&changedCandidates, &changeCount](
            std::vector<KiriView::ImageNavigationCandidate> candidates) {
            changedCandidates = std::move(candidates);
            ++changeCount;
        },
        [](const QString &) {},
        [&state, &cancelCount](QObject *token) {
            ++cancelCount;
            state.removeSubscriber(token);
        });

    KiriView::ImageCandidateStoreEntryNotificationPlan plan
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

void TestImageCandidateStoreEntryState::
    failedListingReturnsErrorPlansForPendingLoadsAndSubscribers()
{
    KiriView::ImageCandidateStoreEntryState state;
    QString pendingError;
    QString subscriberError;
    int loadCount = 0;
    int subscriberChangeCount = 0;

    KiriView::ImageIoJob pending = state.addPendingLoad(
        this, this, [&loadCount](std::vector<KiriView::ImageNavigationCandidate>) { ++loadCount; },
        [&pendingError](const QString &errorString) { pendingError = errorString; },
        [&state](QObject *token) { state.removePendingLoad(token); });
    KiriView::ImageIoJob subscriber = state.addSubscriber(
        this, this,
        [&subscriberChangeCount](
            std::vector<KiriView::ImageNavigationCandidate>) { ++subscriberChangeCount; },
        [&subscriberError](const QString &errorString) { subscriberError = errorString; },
        [&state](QObject *token) { state.removeSubscriber(token); });

    const KiriView::ImageCandidateStoreEntryNotificationPlan plan
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

QTEST_GUILESS_MAIN(TestImageCandidateStoreEntryState)

#include "test_imagecandidatestoreentrystate.moc"
