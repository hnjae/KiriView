// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagecandidatestore.h"

#include <KDirNotify>
#include <QFile>
#include <QList>
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

std::vector<QUrl> candidateUrls(const std::vector<KiriView::ImageNavigationCandidate> &candidates)
{
    std::vector<QUrl> urls;
    urls.reserve(candidates.size());
    for (const KiriView::ImageNavigationCandidate &candidate : candidates) {
        urls.push_back(candidate.url);
    }
    return urls;
}
}

class TestImageCandidateStore : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void localDirectoryPublishesAddedAndDeletedImages();
    void liveDirectoryWatchJobCanOutliveStore();
};

void TestImageCandidateStore::localDirectoryPublishesAddedAndDeletedImages()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(createFile(directory, QStringLiteral("01.png")));

    KiriView::ImageCandidateStore store;
    std::vector<KiriView::ImageNavigationCandidate> loadedCandidates;
    QString loadError;
    bool loaded = false;
    KiriView::ImageIoJob loadJob = store.loadDirectoryImages(
        this, directoryUrl(directory),
        [&loadedCandidates, &loaded](std::vector<KiriView::ImageNavigationCandidate> candidates) {
            loadedCandidates = std::move(candidates);
            loaded = true;
        },
        [&loadError](const QString &errorString) { loadError = errorString; });

    QTRY_VERIFY_WITH_TIMEOUT(loaded || !loadError.isEmpty(), 10000);
    QVERIFY2(loadError.isEmpty(), qPrintable(loadError));
    QCOMPARE(static_cast<int>(loadedCandidates.size()), 1);
    QCOMPARE(loadedCandidates.front().url, fileUrl(directory, QStringLiteral("01.png")));

    std::vector<KiriView::ImageNavigationCandidate> changedCandidates;
    int changeCount = 0;
    KiriView::ImageIoJob watchJob = store.watchDirectoryImages(
        this, directoryUrl(directory),
        [&changedCandidates, &changeCount](
            std::vector<KiriView::ImageNavigationCandidate> candidates) {
            changedCandidates = std::move(candidates);
            ++changeCount;
        },
        [](const QString &) {});

    QVERIFY(createFile(directory, QStringLiteral("02.png")));
    const std::vector<QUrl> addedUrls {
        fileUrl(directory, QStringLiteral("01.png")),
        fileUrl(directory, QStringLiteral("02.png")),
    };
    QTRY_VERIFY_WITH_TIMEOUT(candidateUrls(changedCandidates) == addedUrls, 10000);
    QCOMPARE(changeCount, 1);

    std::vector<KiriView::ImageNavigationCandidate> cachedCandidates;
    store.loadDirectoryImages(
        this, directoryUrl(directory),
        [&cachedCandidates](std::vector<KiriView::ImageNavigationCandidate> candidates) {
            cachedCandidates = std::move(candidates);
        },
        [](const QString &) {});
    QVERIFY(candidateUrls(cachedCandidates) == addedUrls);

    QVERIFY(QFile::remove(directory.filePath(QStringLiteral("01.png"))));
    OrgKdeKDirNotifyInterface::emitFilesRemoved(
        QList<QUrl> { fileUrl(directory, QStringLiteral("01.png")) });
    const std::vector<QUrl> deletedUrls { fileUrl(directory, QStringLiteral("02.png")) };
    QTRY_VERIFY_WITH_TIMEOUT(candidateUrls(changedCandidates) == deletedUrls, 10000);
    QCOMPARE(changeCount, 2);

    watchJob.cancel();
}

void TestImageCandidateStore::liveDirectoryWatchJobCanOutliveStore()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    KiriView::ImageIoJob watchJob;
    {
        KiriView::ImageCandidateStore store;
        watchJob = store.watchDirectoryImages(
            this, directoryUrl(directory), [](std::vector<KiriView::ImageNavigationCandidate>) { },
            [](const QString &) { });
        QVERIFY(watchJob.isActive());
    }

    QVERIFY(watchJob.isActive());
    watchJob.cancel();
    QVERIFY(!watchJob.isActive());
}

QTEST_GUILESS_MAIN(TestImageCandidateStore)

#include "test_imagecandidatestore.moc"
