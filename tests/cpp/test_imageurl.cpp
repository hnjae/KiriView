// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagelocation.h"
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
    void imageLocationTypesExposeExplicitState();
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

void TestImageUrl::imageLocationTypesExposeExplicitState()
{
    const KiriView::DisplayedImageLocation emptyLocation;
    QVERIFY(emptyLocation.isEmpty());

    const KiriView::DisplayedImageLocation location {
        QUrl::fromLocalFile(QStringLiteral("/images/page.png")),
        QUrl(QStringLiteral("zip:///books/book.cbz/")),
    };
    QVERIFY(!location.isEmpty());

    const KiriView::ImageLoadRequest plainOpen
        = KiriView::ImageLoadRequest::fromUrls(location.imageUrl, QUrl());
    QVERIFY(!plainOpen.isEmpty());
    QVERIFY(!plainOpen.isContainerNavigation());
    QCOMPARE(plainOpen.sourceUrl(), location.imageUrl);

    const QUrl containerUrl = QUrl::fromLocalFile(QStringLiteral("/images/"));
    const KiriView::ImageLoadRequest containerOpen = KiriView::ImageLoadRequest::fromUrls(
        location.imageUrl, location.comicBookRootUrl, containerUrl);
    QVERIFY(containerOpen.isContainerNavigation());
    QCOMPARE(containerOpen.containerNavigationUrl(), containerUrl);
}

QTEST_GUILESS_MAIN(TestImageUrl)

#include "test_imageurl.moc"
