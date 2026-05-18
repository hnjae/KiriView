// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidatedirectoryentry.h"

#include <KDirNotify>
#include <QFile>
#include <QList>
#include <QObject>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <utility>
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

class TestImageCandidateDirectoryEntry : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void listerCompletionPublishesPendingLoad();
    void listerChangesPublishSubscriberUpdates();
};

void TestImageCandidateDirectoryEntry::listerCompletionPublishesPendingLoad()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(createFile(directory, QStringLiteral("01.png")));

    KiriView::ImageCandidateDirectoryEntry *entryPtr = nullptr;
    QString completedKey;
    QString errorKey;
    QString errorString;
    KiriView::ImageCandidateDirectoryEntry entry(directoryUrl(directory), QStringLiteral("entry"),
        this,
        KiriView::ImageCandidateDirectoryEntry::Callbacks {
            [&entryPtr, &completedKey](const QString &key) {
                completedKey = key;
                entryPtr->handleCompleted();
            },
            [&entryPtr](const QString &) { entryPtr->handleChanged(); },
            [&entryPtr, &errorKey, &errorString](const QString &key, const QString &message) {
                errorKey = key;
                errorString = message;
                entryPtr->handleError(message);
            },
        });
    entryPtr = &entry;

    std::vector<KiriView::ImageNavigationCandidate> loadedCandidates;
    bool loaded = false;
    KiriView::ImageIoJob loadJob = entry.addPendingLoad(
        [&loadedCandidates, &loaded](std::vector<KiriView::ImageNavigationCandidate> candidates) {
            loadedCandidates = std::move(candidates);
            loaded = true;
        },
        [&errorString](const QString &message) { errorString = message; }, this, [](QObject *) {});

    QVERIFY(loadJob.isActive());
    QVERIFY(entry.open());

    QTRY_VERIFY_WITH_TIMEOUT(loaded || !errorString.isEmpty(), 10000);
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(completedKey, QStringLiteral("entry"));
    QVERIFY(errorKey.isEmpty());
    QCOMPARE(candidateUrls(loadedCandidates),
        std::vector<QUrl>({ fileUrl(directory, QStringLiteral("01.png")) }));
    QVERIFY(entry.listed());
    QVERIFY(!entry.failed());
}

void TestImageCandidateDirectoryEntry::listerChangesPublishSubscriberUpdates()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(createFile(directory, QStringLiteral("01.png")));

    KiriView::ImageCandidateDirectoryEntry *entryPtr = nullptr;
    KiriView::ImageCandidateDirectoryEntry entry(directoryUrl(directory), QStringLiteral("entry"),
        this,
        KiriView::ImageCandidateDirectoryEntry::Callbacks {
            [&entryPtr](const QString &) { entryPtr->handleCompleted(); },
            [&entryPtr](const QString &) { entryPtr->handleChanged(); },
            [&entryPtr](
                const QString &, const QString &message) { entryPtr->handleError(message); },
        });
    entryPtr = &entry;

    bool loaded = false;
    QString errorString;
    KiriView::ImageIoJob loadJob = entry.addPendingLoad(
        [&loaded](std::vector<KiriView::ImageNavigationCandidate>) { loaded = true; },
        [&errorString](const QString &message) { errorString = message; }, this, [](QObject *) {});
    QVERIFY(loadJob.isActive());
    QVERIFY(entry.open());
    QTRY_VERIFY_WITH_TIMEOUT(loaded || !errorString.isEmpty(), 10000);
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));

    std::vector<KiriView::ImageNavigationCandidate> changedCandidates;
    int changeCount = 0;
    KiriView::ImageIoJob watchJob = entry.addSubscriber(
        [&changedCandidates, &changeCount](
            std::vector<KiriView::ImageNavigationCandidate> candidates) {
            changedCandidates = std::move(candidates);
            ++changeCount;
        },
        [&errorString](const QString &message) { errorString = message; }, this, [](QObject *) {});

    QVERIFY(watchJob.isActive());
    QVERIFY(createFile(directory, QStringLiteral("02.png")));
    OrgKdeKDirNotifyInterface::emitFilesAdded(directoryUrl(directory));

    QCOMPARE(candidateUrls(changedCandidates), std::vector<QUrl>());
    const std::vector<QUrl> expectedAddedUrls {
        fileUrl(directory, QStringLiteral("01.png")),
        fileUrl(directory, QStringLiteral("02.png")),
    };
    QTRY_VERIFY_WITH_TIMEOUT(candidateUrls(changedCandidates) == expectedAddedUrls, 10000);
    QCOMPARE(changeCount, 1);

    QVERIFY(QFile::remove(directory.filePath(QStringLiteral("01.png"))));
    OrgKdeKDirNotifyInterface::emitFilesRemoved(
        QList<QUrl> { fileUrl(directory, QStringLiteral("01.png")) });
    const std::vector<QUrl> expectedDeletedUrls { fileUrl(directory, QStringLiteral("02.png")) };
    QTRY_VERIFY_WITH_TIMEOUT(candidateUrls(changedCandidates) == expectedDeletedUrls, 10000);
    QCOMPARE(changeCount, 2);
}

QTEST_GUILESS_MAIN(TestImageCandidateDirectoryEntry)

#include "test_imagecandidatedirectoryentry.moc"
