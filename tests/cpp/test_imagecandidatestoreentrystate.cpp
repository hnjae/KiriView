// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidatestoreentrystate.h"

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
}

class TestImageCandidateStoreEntryState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void completedListingFinishesPendingLoadsAndCachesCandidates();
    void cancelledPendingLoadDoesNotReceiveCompletion();
    void subscribersOnlyReceiveChangesAfterInitialListing();
    void failedListingFinishesPendingErrorsAndNotifiesSubscribers();
};

void TestImageCandidateStoreEntryState::completedListingFinishesPendingLoadsAndCachesCandidates()
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
    state.completeListing(candidates({ QStringLiteral("01.png"), QStringLiteral("02.png") }));

    QVERIFY(state.listed());
    QVERIFY(!state.failed());
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
    state.completeListing(candidates({ QStringLiteral("01.png") }));

    QCOMPARE(cancelCount, 1);
    QCOMPARE(loadCount, 0);
    QVERIFY(!job.isActive());
}

void TestImageCandidateStoreEntryState::subscribersOnlyReceiveChangesAfterInitialListing()
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

    state.completeListing(candidates({ QStringLiteral("01.png") }));
    QCOMPARE(changeCount, 0);

    state.updateListing(candidates({ QStringLiteral("01.png") }));
    QCOMPARE(changeCount, 0);

    state.updateListing(candidates({ QStringLiteral("01.png"), QStringLiteral("02.png") }));
    QCOMPARE(changeCount, 1);
    QCOMPARE(changedCandidates.size(), std::size_t(2));

    job.cancel();
    state.updateListing(candidates(
        { QStringLiteral("01.png"), QStringLiteral("02.png"), QStringLiteral("03.png") }));
    QCOMPARE(cancelCount, 1);
    QCOMPARE(changeCount, 1);
}

void TestImageCandidateStoreEntryState::failedListingFinishesPendingErrorsAndNotifiesSubscribers()
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

    state.failListing(QStringLiteral("missing directory"));

    QVERIFY(state.failed());
    QCOMPARE(state.errorString(), QStringLiteral("missing directory"));
    QVERIFY(!pending.isActive());
    QVERIFY(subscriber.isActive());
    QCOMPARE(loadCount, 0);
    QCOMPARE(subscriberChangeCount, 0);
    QCOMPARE(pendingError, QStringLiteral("missing directory"));
    QCOMPARE(subscriberError, QStringLiteral("missing directory"));
}

QTEST_GUILESS_MAIN(TestImageCandidateStoreEntryState)

#include "test_imagecandidatestoreentrystate.moc"
