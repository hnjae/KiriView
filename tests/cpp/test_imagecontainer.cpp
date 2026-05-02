// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainer.h"
#include "imageurl.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>

class TestImageContainer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void comicBookArchiveRootUrlsUseZipScheme();
    void archiveInteriorUrlsResolveToTheirRootAndTitle();
    void archiveImageContainerUrlsResolveToArchiveFile();
    void regularImageContainerUrlsResolveToParentDirectory();
};

void TestImageContainer::comicBookArchiveRootUrlsUseZipScheme()
{
    const QUrl archiveUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(archiveUrl);

    QVERIFY(archiveRootUrl.has_value());
    QCOMPARE(archiveRootUrl->scheme(), QStringLiteral("zip"));
    QCOMPARE(archiveRootUrl->path(), QStringLiteral("/books/book.cbz/"));
}

void TestImageContainer::archiveInteriorUrlsResolveToTheirRootAndTitle()
{
    const QUrl archiveUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());

    QUrl pageUrl = *archiveRootUrl;
    pageUrl.setPath(archiveRootUrl->path() + QStringLiteral("chapter/page001.png"));

    QVERIFY(KiriView::isUrlInsideArchiveRoot(pageUrl, *archiveRootUrl));
    QCOMPARE(KiriView::containingComicBookArchiveRootUrl(pageUrl), archiveRootUrl);
    QCOMPARE(KiriView::windowTitleFileNameForDisplayedUrl(pageUrl, *archiveRootUrl),
        QStringLiteral("book.cbz"));
}

void TestImageContainer::archiveImageContainerUrlsResolveToArchiveFile()
{
    const QUrl archiveUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());

    QUrl pageUrl = *archiveRootUrl;
    pageUrl.setPath(archiveRootUrl->path() + QStringLiteral("chapter/page001.png"));

    QCOMPARE(KiriView::containerNavigationUrlForImage(pageUrl, *archiveRootUrl), archiveUrl);
}

void TestImageContainer::regularImageContainerUrlsResolveToParentDirectory()
{
    const QUrl fileUrl = QUrl::fromLocalFile(QStringLiteral("/images/page.png"));
    const QUrl parentUrl = QUrl::fromLocalFile(QStringLiteral("/images/"));

    QVERIFY(KiriView::sameContainerNavigationUrl(
        KiriView::containerNavigationUrlForImage(fileUrl, QUrl()), parentUrl));
}

QTEST_GUILESS_MAIN(TestImageContainer)

#include "test_imagecontainer.moc"
