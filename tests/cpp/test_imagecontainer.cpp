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
    void comicBookArchiveRootUrlsUseFormatSpecificKioSchemes();
    void directArchiveRootUrlsUseFormatSpecificKioSchemes();
    void archiveInteriorUrlsResolveToTheirRootAndTitle();
    void directArchiveInteriorUrlsResolveToTheirRootAndTitle();
    void cbtAndCb7InteriorUrlsResolveToTheirRoots();
    void archiveImageContainerUrlsResolveToArchiveFile();
    void directArchivePagesDoNotResolveToContainerNavigationUrls();
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

void TestImageContainer::comicBookArchiveRootUrlsUseFormatSpecificKioSchemes()
{
    const std::optional<QUrl> cbtRootUrl
        = KiriView::comicBookArchiveRootUrl(QUrl::fromLocalFile(QStringLiteral("/books/book.cbt")));
    QVERIFY(cbtRootUrl.has_value());
    QCOMPARE(cbtRootUrl->scheme(), QStringLiteral("tar"));
    QCOMPARE(cbtRootUrl->path(), QStringLiteral("/books/book.cbt/"));

    const std::optional<QUrl> cb7RootUrl
        = KiriView::comicBookArchiveRootUrl(QUrl::fromLocalFile(QStringLiteral("/books/book.cb7")));
    QVERIFY(cb7RootUrl.has_value());
    QCOMPARE(cb7RootUrl->scheme(), QStringLiteral("sevenz"));
    QCOMPARE(cb7RootUrl->path(), QStringLiteral("/books/book.cb7/"));
}

void TestImageContainer::directArchiveRootUrlsUseFormatSpecificKioSchemes()
{
    const std::optional<QUrl> zipRootUrl = KiriView::directArchiveOpenRootUrl(
        QUrl::fromLocalFile(QStringLiteral("/books/book.zip")));
    QVERIFY(zipRootUrl.has_value());
    QCOMPARE(zipRootUrl->scheme(), QStringLiteral("zip"));
    QCOMPARE(zipRootUrl->path(), QStringLiteral("/books/book.zip/"));

    const std::optional<QUrl> tarRootUrl = KiriView::directArchiveOpenRootUrl(
        QUrl::fromLocalFile(QStringLiteral("/books/book.tar")));
    QVERIFY(tarRootUrl.has_value());
    QCOMPARE(tarRootUrl->scheme(), QStringLiteral("tar"));
    QCOMPARE(tarRootUrl->path(), QStringLiteral("/books/book.tar/"));

    const std::optional<QUrl> sevenZipRootUrl
        = KiriView::directArchiveOpenRootUrl(QUrl::fromLocalFile(QStringLiteral("/books/book.7z")));
    QVERIFY(sevenZipRootUrl.has_value());
    QCOMPARE(sevenZipRootUrl->scheme(), QStringLiteral("sevenz"));
    QCOMPARE(sevenZipRootUrl->path(), QStringLiteral("/books/book.7z/"));
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

void TestImageContainer::directArchiveInteriorUrlsResolveToTheirRootAndTitle()
{
    const QUrl archiveUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.zip"));
    const std::optional<QUrl> archiveRootUrl = KiriView::directArchiveOpenRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());

    QUrl pageUrl = *archiveRootUrl;
    pageUrl.setPath(archiveRootUrl->path() + QStringLiteral("chapter/page001.png"));

    QVERIFY(KiriView::isUrlInsideArchiveRoot(pageUrl, *archiveRootUrl));
    QCOMPARE(KiriView::containingDirectArchiveOpenRootUrl(pageUrl), archiveRootUrl);
    QCOMPARE(KiriView::windowTitleFileNameForDisplayedUrl(pageUrl, *archiveRootUrl),
        QStringLiteral("book.zip"));
}

void TestImageContainer::cbtAndCb7InteriorUrlsResolveToTheirRoots()
{
    const QUrl cbtRootUrl(QStringLiteral("tar:///books/book.cbt/"));
    QUrl cbtPageUrl = cbtRootUrl;
    cbtPageUrl.setPath(cbtRootUrl.path() + QStringLiteral("chapter/page001.png"));
    QCOMPARE(KiriView::containingComicBookArchiveRootUrl(cbtPageUrl), cbtRootUrl);
    QCOMPARE(KiriView::windowTitleFileNameForDisplayedUrl(cbtPageUrl, cbtRootUrl),
        QStringLiteral("book.cbt"));

    const QUrl cb7RootUrl(QStringLiteral("sevenz:///books/book.cb7/"));
    QUrl cb7PageUrl = cb7RootUrl;
    cb7PageUrl.setPath(cb7RootUrl.path() + QStringLiteral("chapter/page001.png"));
    QCOMPARE(KiriView::containingComicBookArchiveRootUrl(cb7PageUrl), cb7RootUrl);
    QCOMPARE(KiriView::windowTitleFileNameForDisplayedUrl(cb7PageUrl, cb7RootUrl),
        QStringLiteral("book.cb7"));
}

void TestImageContainer::archiveImageContainerUrlsResolveToArchiveFile()
{
    const QUrl archiveUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());

    QUrl pageUrl = *archiveRootUrl;
    pageUrl.setPath(archiveRootUrl->path() + QStringLiteral("chapter/page001.png"));

    QCOMPARE(KiriView::containerNavigationUrlForImage(pageUrl, *archiveRootUrl), archiveUrl);
    QCOMPARE(KiriView::imageContainerUrlForImage(pageUrl, *archiveRootUrl), archiveUrl);
}

void TestImageContainer::directArchivePagesDoNotResolveToContainerNavigationUrls()
{
    const QUrl archiveUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.zip"));
    const std::optional<QUrl> archiveRootUrl = KiriView::directArchiveOpenRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());

    QUrl pageUrl = *archiveRootUrl;
    pageUrl.setPath(archiveRootUrl->path() + QStringLiteral("chapter/page001.png"));

    QVERIFY(KiriView::containerNavigationUrlForImage(pageUrl, *archiveRootUrl).isEmpty());
    QCOMPARE(KiriView::imageContainerUrlForImage(pageUrl, *archiveRootUrl), archiveUrl);
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
