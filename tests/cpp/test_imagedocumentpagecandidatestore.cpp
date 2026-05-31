// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagedocumentpagecandidatestore.h"

#include <KDirNotify>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <vector>

namespace {
QUrl directoryUrl(const QTemporaryDir &directory)
{
    return QUrl::fromLocalFile(directory.path() + QLatin1Char('/'));
}

QUrl fileUrl(const QTemporaryDir &directory, const QString &fileName)
{
    return QUrl::fromLocalFile(directory.filePath(fileName));
}

bool createFile(const QTemporaryDir &directory, const QString &fileName)
{
    QFile file(directory.filePath(fileName));
    return file.open(QIODevice::WriteOnly);
}

std::vector<QUrl> candidateUrls(const std::vector<KiriView::ImageDocumentPageCandidate> &candidates)
{
    std::vector<QUrl> urls;
    urls.reserve(candidates.size());
    for (const KiriView::ImageDocumentPageCandidate &candidate : candidates) {
        urls.push_back(candidate.url);
    }
    return urls;
}
}

class TestImageDocumentPageCandidateStore : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void localDirectoryPublishesAddedImagesAndReusesCache();
    void liveDirectoryWatchJobCanOutliveStore();
};

void TestImageDocumentPageCandidateStore::localDirectoryPublishesAddedImagesAndReusesCache()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(createFile(directory, QStringLiteral("01.png")));

    KiriView::ImageDocumentPageCandidateStore store;
    std::vector<KiriView::ImageDocumentPageCandidate> loadedCandidates;
    QString loadError;
    bool loaded = false;
    KiriView::ImageIoJob loadJob = store.loadDirectoryImages(
        this, directoryUrl(directory),
        [&loadedCandidates, &loaded](std::vector<KiriView::ImageDocumentPageCandidate> candidates) {
            loadedCandidates = std::move(candidates);
            loaded = true;
        },
        [&loadError](const QString &errorString) { loadError = errorString; });

    QTRY_VERIFY_WITH_TIMEOUT(loaded || !loadError.isEmpty(), 10000);
    QVERIFY2(loadError.isEmpty(), qPrintable(loadError));
    QCOMPARE(static_cast<int>(loadedCandidates.size()), 1);
    QCOMPARE(loadedCandidates.front().url, fileUrl(directory, QStringLiteral("01.png")));

    std::vector<KiriView::ImageDocumentPageCandidate> changedCandidates;
    int changeCount = 0;
    KiriView::ImageIoJob watchJob = store.watchDirectoryImages(
        this, directoryUrl(directory),
        [&changedCandidates, &changeCount](
            std::vector<KiriView::ImageDocumentPageCandidate> candidates) {
            changedCandidates = std::move(candidates);
            ++changeCount;
        },
        [](const QString &) {});

    QVERIFY(createFile(directory, QStringLiteral("02.png")));
    OrgKdeKDirNotifyInterface::emitFilesAdded(directoryUrl(directory));
    const std::vector<QUrl> addedUrls {
        fileUrl(directory, QStringLiteral("01.png")),
        fileUrl(directory, QStringLiteral("02.png")),
    };
    QTRY_VERIFY_WITH_TIMEOUT(candidateUrls(changedCandidates) == addedUrls, 10000);
    QCOMPARE(changeCount, 1);

    std::vector<KiriView::ImageDocumentPageCandidate> cachedCandidates;
    store.loadDirectoryImages(
        this, directoryUrl(directory),
        [&cachedCandidates](std::vector<KiriView::ImageDocumentPageCandidate> candidates) {
            cachedCandidates = std::move(candidates);
        },
        [](const QString &) {});
    QVERIFY(candidateUrls(cachedCandidates) == addedUrls);

    watchJob.cancel();
}

void TestImageDocumentPageCandidateStore::liveDirectoryWatchJobCanOutliveStore()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    KiriView::ImageIoJob watchJob;
    {
        KiriView::ImageDocumentPageCandidateStore store;
        watchJob = store.watchDirectoryImages(
            this, directoryUrl(directory),
            [](std::vector<KiriView::ImageDocumentPageCandidate>) { }, [](const QString &) { });
        QVERIFY(watchJob.isActive());
    }

    QVERIFY(watchJob.isActive());
    watchJob.cancel();
    QVERIFY(!watchJob.isActive());
}

QTEST_GUILESS_MAIN(TestImageDocumentPageCandidateStore)

#include "test_imagedocumentpagecandidatestore.moc"
