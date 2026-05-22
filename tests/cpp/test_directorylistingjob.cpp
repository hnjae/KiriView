// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "async/directorylistingjob.h"

#include <KFileItem>
#include <QFile>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <utility>

namespace {
QUrl directoryUrl(const QTemporaryDir &directory)
{
    return QUrl::fromLocalFile(directory.path() + QLatin1Char('/'));
}

bool createFile(const QTemporaryDir &directory, const QString &fileName)
{
    QFile file(directory.filePath(fileName));
    return file.open(QIODevice::WriteOnly);
}

QStringList itemNames(const KFileItemList &items)
{
    QStringList names;
    for (const KFileItem &item : items) {
        names.push_back(item.name());
    }
    names.sort();
    return names;
}
}

class TestDirectoryListingJob : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void localDirectoryReturnsItemSnapshot();
    void cancelSuppressesCompletion();
};

void TestDirectoryListingJob::localDirectoryReturnsItemSnapshot()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(createFile(directory, QStringLiteral("01.png")));
    QVERIFY(createFile(directory, QStringLiteral("clip.mp4")));

    KFileItemList listedItems;
    QString errorString;
    bool listed = false;
    KiriView::ImageIoJob job = KiriView::startDirectoryItemList(
        this, directoryUrl(directory),
        [&listedItems, &listed](KFileItemList items) {
            listedItems = std::move(items);
            listed = true;
        },
        [&errorString](const QString &message) { errorString = message; });

    QTRY_VERIFY_WITH_TIMEOUT(listed || !errorString.isEmpty(), 10000);
    QVERIFY2(errorString.isEmpty(), qPrintable(errorString));
    QVERIFY(!job.isActive());
    QCOMPARE(itemNames(listedItems),
        QStringList({ QStringLiteral("01.png"), QStringLiteral("clip.mp4") }));
}

void TestDirectoryListingJob::cancelSuppressesCompletion()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(createFile(directory, QStringLiteral("01.png")));

    bool listed = false;
    QString errorString;
    KiriView::ImageIoJob job = KiriView::startDirectoryItemList(
        this, directoryUrl(directory), [&listed](KFileItemList) { listed = true; },
        [&errorString](const QString &message) { errorString = message; });

    QVERIFY(job.isActive());
    job.cancel();
    QVERIFY(!job.isActive());

    QTest::qWait(100);
    QVERIFY(!listed);
    QVERIFY(errorString.isEmpty());
}

QTEST_GUILESS_MAIN(TestDirectoryListingJob)

#include "test_directorylistingjob.moc"
