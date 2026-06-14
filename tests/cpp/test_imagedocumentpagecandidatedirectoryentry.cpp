// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagedocumentpagecandidatedirectoryentry.h"

#include <QList>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <utility>
#include <vector>

namespace {
QUrl directoryUrl()
{
    return QUrl::fromLocalFile(QStringLiteral("/tmp/kiriview-directory-entry-test/"));
}

QUrl fileUrl(const QString &fileName)
{
    return QUrl::fromLocalFile(
        QStringLiteral("/tmp/kiriview-directory-entry-test/%1").arg(fileName));
}

kiriview::ImageDocumentPageCandidate candidate(const QString &fileName)
{
    return kiriview::ImageDocumentPageCandidate {
        fileUrl(fileName),
        fileName,
        kiriview::ImageDocumentPageKind::Image,
    };
}

std::vector<QUrl> candidateUrls(const std::vector<kiriview::ImageDocumentPageCandidate> &candidates)
{
    std::vector<QUrl> urls;
    urls.reserve(candidates.size());
    for (const kiriview::ImageDocumentPageCandidate &candidate : candidates) {
        urls.push_back(candidate.url);
    }
    return urls;
}

struct FakeWatchProvider {
    QUrl watchedUrl;
    kiriview::ImageDocumentPageCandidateWatchSnapshotCallback initialSnapshot;
    kiriview::ImageDocumentPageCandidateWatchSnapshotCallback changedSnapshot;
    kiriview::ImageDocumentPageCandidateWatchDeletedCallback deletedUrls;
    kiriview::ErrorCallback errorCallback;
    int startCount = 0;

    kiriview::ImageDocumentPageCandidateWatchProvider provider()
    {
        return [this](QObject *receiver, QUrl directory,
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
            auto *token = new QObject(receiver);
            return kiriview::ImageIoJob(token, [](QObject *object) { object->deleteLater(); });
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

    void remove(QList<QUrl> urls)
    {
        if (deletedUrls) {
            deletedUrls(std::move(urls));
        }
    }

    void fail(const QString &message)
    {
        if (errorCallback) {
            errorCallback(message);
        }
    }
};
}

class TestImageDocumentPageCandidateDirectoryEntry : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void providerCompletionPublishesPendingLoad();
    void providerChangesPublishSubscriberUpdates();
    void providerFailurePublishesPendingLoadError();
};

void TestImageDocumentPageCandidateDirectoryEntry::providerCompletionPublishesPendingLoad()
{
    FakeWatchProvider provider;
    QString errorString;
    kiriview::ImageDocumentPageCandidateDirectoryEntry entry(
        directoryUrl(), provider.provider(), this);

    std::vector<kiriview::ImageDocumentPageCandidate> loadedCandidates;
    bool loaded = false;
    kiriview::ImageIoJob loadJob = entry.addPendingLoad(
        [&loadedCandidates, &loaded](std::vector<kiriview::ImageDocumentPageCandidate> candidates) {
            loadedCandidates = std::move(candidates);
            loaded = true;
        },
        [&errorString](const QString &message) { errorString = message; }, this, [](QObject *) {});

    QVERIFY(loadJob.isActive());
    QVERIFY(entry.open());
    QCOMPARE(provider.startCount, 1);
    QCOMPARE(provider.watchedUrl, directoryUrl());

    provider.complete({ candidate(QStringLiteral("01.png")) });
    QVERIFY(loaded);
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(
        candidateUrls(loadedCandidates), std::vector<QUrl>({ fileUrl(QStringLiteral("01.png")) }));
    QVERIFY(entry.listed());
    QVERIFY(!entry.failed());
}

void TestImageDocumentPageCandidateDirectoryEntry::providerChangesPublishSubscriberUpdates()
{
    FakeWatchProvider provider;
    QString errorString;
    kiriview::ImageDocumentPageCandidateDirectoryEntry entry(
        directoryUrl(), provider.provider(), this);

    bool loaded = false;
    kiriview::ImageIoJob loadJob = entry.addPendingLoad(
        [&loaded](std::vector<kiriview::ImageDocumentPageCandidate>) { loaded = true; },
        [&errorString](const QString &message) { errorString = message; }, this, [](QObject *) {});
    QVERIFY(loadJob.isActive());
    QVERIFY(entry.open());
    provider.complete({ candidate(QStringLiteral("01.png")) });
    QVERIFY(loaded);
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    std::vector<kiriview::ImageDocumentPageCandidate> changedCandidates;
    int changeCount = 0;
    kiriview::ImageIoJob watchJob = entry.addSubscriber(
        [&changedCandidates, &changeCount](
            std::vector<kiriview::ImageDocumentPageCandidate> candidates) {
            changedCandidates = std::move(candidates);
            ++changeCount;
        },
        [&errorString](const QString &message) { errorString = message; }, this, [](QObject *) {});

    QVERIFY(watchJob.isActive());
    provider.change({ candidate(QStringLiteral("01.png")), candidate(QStringLiteral("02.png")) });
    const std::vector<QUrl> expectedAddedUrls {
        fileUrl(QStringLiteral("01.png")),
        fileUrl(QStringLiteral("02.png")),
    };
    QCOMPARE(candidateUrls(changedCandidates), expectedAddedUrls);
    QCOMPARE(changeCount, 1);

    provider.remove(QList<QUrl> { fileUrl(QStringLiteral("01.png")) });
    const std::vector<QUrl> expectedDeletedUrls { fileUrl(QStringLiteral("02.png")) };
    QCOMPARE(candidateUrls(changedCandidates), expectedDeletedUrls);
    QCOMPARE(changeCount, 2);
}

void TestImageDocumentPageCandidateDirectoryEntry::providerFailurePublishesPendingLoadError()
{
    FakeWatchProvider provider;
    QString errorString;
    kiriview::ImageDocumentPageCandidateDirectoryEntry entry(
        directoryUrl(), provider.provider(), this);

    bool loaded = false;
    kiriview::ImageIoJob loadJob = entry.addPendingLoad(
        [&loaded](std::vector<kiriview::ImageDocumentPageCandidate>) { loaded = true; },
        [&errorString](const QString &message) { errorString = message; }, this, [](QObject *) {});

    QVERIFY(loadJob.isActive());
    QVERIFY(entry.open());
    provider.fail(QStringLiteral("watch failed"));

    QVERIFY(!loaded);
    QCOMPARE(errorString, QStringLiteral("watch failed"));
    QVERIFY(entry.failed());
    QCOMPARE(entry.errorString(), QStringLiteral("watch failed"));
}

QTEST_GUILESS_MAIN(TestImageDocumentPageCandidateDirectoryEntry)

#include "test_imagedocumentpagecandidatedirectoryentry.moc"
