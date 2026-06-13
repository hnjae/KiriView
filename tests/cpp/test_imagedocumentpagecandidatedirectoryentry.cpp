// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagedocumentpagecandidatedirectoryentry.h"

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

std::vector<QUrl> candidateUrls(const std::vector<kiriview::ImageDocumentPageCandidate> &candidates)
{
    std::vector<QUrl> urls;
    urls.reserve(candidates.size());
    for (const kiriview::ImageDocumentPageCandidate &candidate : candidates) {
        urls.push_back(candidate.url);
    }
    return urls;
}
}

class TestImageDocumentPageCandidateDirectoryEntry : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void listerCompletionPublishesPendingLoad();
    void listerChangesPublishSubscriberUpdates();
};

void TestImageDocumentPageCandidateDirectoryEntry::listerCompletionPublishesPendingLoad()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(createFile(directory, QStringLiteral("01.png")));

    QString errorString;
    kiriview::ImageDocumentPageCandidateDirectoryEntry entry(directoryUrl(directory), this);

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

    QTRY_VERIFY_WITH_TIMEOUT(loaded || !errorString.isEmpty(), 10000);
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QCOMPARE(candidateUrls(loadedCandidates),
        std::vector<QUrl>({ fileUrl(directory, QStringLiteral("01.png")) }));
    QVERIFY(entry.listed());
    QVERIFY(!entry.failed());
}

void TestImageDocumentPageCandidateDirectoryEntry::listerChangesPublishSubscriberUpdates()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(createFile(directory, QStringLiteral("01.png")));

    kiriview::ImageDocumentPageCandidateDirectoryEntry entry(directoryUrl(directory), this);

    bool loaded = false;
    QString errorString;
    kiriview::ImageIoJob loadJob = entry.addPendingLoad(
        [&loaded](std::vector<kiriview::ImageDocumentPageCandidate>) { loaded = true; },
        [&errorString](const QString &message) { errorString = message; }, this, [](QObject *) {});
    QVERIFY(loadJob.isActive());
    QVERIFY(entry.open());
    QTRY_VERIFY_WITH_TIMEOUT(loaded || !errorString.isEmpty(), 10000);
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
    entry.handleDeleted(QList<QUrl> { fileUrl(directory, QStringLiteral("01.png")) });
    const std::vector<QUrl> expectedDeletedUrls { fileUrl(directory, QStringLiteral("02.png")) };
    QCOMPARE(candidateUrls(changedCandidates), expectedDeletedUrls);
    QCOMPARE(changeCount, 2);
}

QTEST_GUILESS_MAIN(TestImageDocumentPageCandidateDirectoryEntry)

#include "test_imagedocumentpagecandidatedirectoryentry.moc"
