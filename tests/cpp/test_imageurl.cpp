// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageurl.h"

#include <QObject>
#include <QTest>
#include <QUrl>

class TestImageUrl : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void normalizedContainerUrlsStripQueryFragmentsAndCleanLocalPaths();
    void sameContainerNavigationUrlMatchesNormalizedPaths();
    void parentUrlForContainerNavigationHandlesContainers();
};

void TestImageUrl::normalizedContainerUrlsStripQueryFragmentsAndCleanLocalPaths()
{
    QUrl fileUrl = QUrl::fromLocalFile(QStringLiteral("/images/./chapter/../page.png"));
    fileUrl.setQuery(QStringLiteral("cache=1"));
    fileUrl.setFragment(QStringLiteral("view"));
    QCOMPARE(KiriView::normalizedFileContainerUrl(fileUrl),
        QUrl::fromLocalFile(QStringLiteral("/images/page.png")));

    QUrl directoryUrl = QUrl::fromLocalFile(QStringLiteral("/images/chapter"));
    directoryUrl.setQuery(QStringLiteral("cache=1"));
    directoryUrl.setFragment(QStringLiteral("view"));
    QCOMPARE(KiriView::normalizedDirectoryContainerUrl(directoryUrl),
        QUrl::fromLocalFile(QStringLiteral("/images/chapter/")));
}

void TestImageUrl::sameContainerNavigationUrlMatchesNormalizedPaths()
{
    QVERIFY(KiriView::sameContainerNavigationUrl(
        QUrl::fromLocalFile(QStringLiteral("/images/chapter/../")),
        QUrl::fromLocalFile(QStringLiteral("/images/"))));
    QVERIFY(!KiriView::sameContainerNavigationUrl(
        QUrl(), QUrl::fromLocalFile(QStringLiteral("/images/"))));
}

void TestImageUrl::parentUrlForContainerNavigationHandlesContainers()
{
    QVERIFY(KiriView::sameContainerNavigationUrl(
        KiriView::parentUrlForContainerNavigation(QUrl::fromLocalFile(QStringLiteral("/images/"))),
        QUrl::fromLocalFile(QStringLiteral("/"))));
    const QUrl archiveUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbz"));
    QCOMPARE(KiriView::parentUrlForContainerNavigation(archiveUrl),
        QUrl::fromLocalFile(QStringLiteral("/books/")));
}

QTEST_GUILESS_MAIN(TestImageUrl)

#include "test_imageurl.moc"
