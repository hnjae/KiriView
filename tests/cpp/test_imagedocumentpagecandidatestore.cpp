// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagedocumentpagecandidatestore.h"

#include <QList>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <utility>
#include <vector>

namespace {
QUrl directoryUrl()
{
    return QUrl::fromLocalFile(QStringLiteral("/tmp/kiriview-candidate-store-test/"));
}

QUrl fileUrl(const QString& fileName)
{
    return QUrl::fromLocalFile(
        QStringLiteral("/tmp/kiriview-candidate-store-test/%1").arg(fileName));
}

kiriview::ImageDocumentPageCandidate candidate(const QString& fileName)
{
    return kiriview::ImageDocumentPageCandidate {
        fileUrl(fileName),
        fileName,
        kiriview::ImageDocumentPageKind::Image,
    };
}

std::vector<QUrl> candidateUrls(const std::vector<kiriview::ImageDocumentPageCandidate>& candidates)
{
    std::vector<QUrl> urls;
    urls.reserve(candidates.size());
    for (const kiriview::ImageDocumentPageCandidate& candidate : candidates) {
        urls.push_back(candidate.url);
    }
    return urls;
}

struct FakeWatchProvider
{
    QUrl watchedUrl;
    kiriview::ImageDocumentPageCandidateWatchSnapshotCallback initialSnapshot;
    kiriview::ImageDocumentPageCandidateWatchSnapshotCallback changedSnapshot;
    kiriview::ImageDocumentPageCandidateWatchDeletedCallback deletedUrls;
    kiriview::ErrorCallback errorCallback;
    int startCount = 0;

    kiriview::ImageDocumentPageCandidateWatchProvider provider()
    {
        return [this](QObject* receiver, QUrl directory,
                   kiriview::ImageDocumentPageCandidateWatchSnapshotCallback initial,
                   kiriview::ImageDocumentPageCandidateWatchSnapshotCallback changed,
                   kiriview::ImageDocumentPageCandidateWatchDeletedCallback deleted,
                   kiriview::ErrorCallback error) {
            ++startCount;
            watchedUrl = std::move(directory);
            initialSnapshot = std::move(initial);
            changedSnapshot = std::move(changed);
            deletedUrls = std::move(deleted);
            errorCallback = std::move(error);
            auto* token = new QObject(receiver);
            return kiriview::ImageIoJob(token, [](QObject* object) { object->deleteLater(); });
        };
    }

    void complete(std::vector<kiriview::ImageDocumentPageCandidate> candidates)
    {
        if (initialSnapshot) {
            initialSnapshot(std::move(candidates));
        }
    }

    void change(std::vector<kiriview::ImageDocumentPageCandidate> candidates)
    {
        if (changedSnapshot) {
            changedSnapshot(std::move(candidates));
        }
    }
};
}

class TestImageDocumentPageCandidateStore : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void localDirectoryPublishesProviderSnapshotsAndReusesCache();
    void liveDirectoryWatchJobCanOutliveStore();
};

void TestImageDocumentPageCandidateStore::localDirectoryPublishesProviderSnapshotsAndReusesCache()
{
    FakeWatchProvider provider;
    kiriview::ImageDocumentPageCandidateStore store(provider.provider());
    std::vector<kiriview::ImageDocumentPageCandidate> loadedCandidates;
    QString loadError;
    bool loaded = false;
    kiriview::ImageIoJob loadJob = store.loadDirectoryImages(
        this, directoryUrl(),
        [&loadedCandidates, &loaded](std::vector<kiriview::ImageDocumentPageCandidate> candidates) {
            loadedCandidates = std::move(candidates);
            loaded = true;
        },
        [&loadError](const QString& errorString) { loadError = errorString; });

    QVERIFY(loadJob.isActive());
    QCOMPARE(provider.startCount, 1);
    QCOMPARE(provider.watchedUrl, directoryUrl());
    provider.complete({ candidate(QStringLiteral("01.png")) });
    QVERIFY(loaded);
    QVERIFY2(loadError.isEmpty(), qPrintable(loadError));
    QCOMPARE(static_cast<int>(loadedCandidates.size()), 1);
    QCOMPARE(loadedCandidates.front().url, fileUrl(QStringLiteral("01.png")));

    std::vector<kiriview::ImageDocumentPageCandidate> changedCandidates;
    int changeCount = 0;
    kiriview::ImageIoJob watchJob = store.watchDirectoryImages(
        this, directoryUrl(),
        [&changedCandidates, &changeCount](
            std::vector<kiriview::ImageDocumentPageCandidate> candidates) {
            changedCandidates = std::move(candidates);
            ++changeCount;
        },
        [](const QString&) {});

    provider.change({ candidate(QStringLiteral("01.png")), candidate(QStringLiteral("02.png")) });
    const std::vector<QUrl> addedUrls {
        fileUrl(QStringLiteral("01.png")),
        fileUrl(QStringLiteral("02.png")),
    };
    QCOMPARE(candidateUrls(changedCandidates), addedUrls);
    QCOMPARE(changeCount, 1);

    std::vector<kiriview::ImageDocumentPageCandidate> cachedCandidates;
    store.loadDirectoryImages(
        this, directoryUrl(),
        [&cachedCandidates](std::vector<kiriview::ImageDocumentPageCandidate> candidates) {
            cachedCandidates = std::move(candidates);
        },
        [](const QString&) {});
    QCOMPARE(candidateUrls(cachedCandidates), addedUrls);
    QCOMPARE(provider.startCount, 1);

    watchJob.cancel();
}

void TestImageDocumentPageCandidateStore::liveDirectoryWatchJobCanOutliveStore()
{
    FakeWatchProvider provider;
    kiriview::ImageIoJob watchJob;
    {
        kiriview::ImageDocumentPageCandidateStore store(provider.provider());
        watchJob = store.watchDirectoryImages(
            this, directoryUrl(), [](std::vector<kiriview::ImageDocumentPageCandidate>) { },
            [](const QString&) { });
        QVERIFY(watchJob.isActive());
    }

    QVERIFY(watchJob.isActive());
    watchJob.cancel();
    QVERIFY(!watchJob.isActive());
}

QTEST_GUILESS_MAIN(TestImageDocumentPageCandidateStore)

#include "test_imagedocumentpagecandidatestore.moc"
