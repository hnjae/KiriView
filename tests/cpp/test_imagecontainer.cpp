// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainer.h"
#include "imageurl.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <sys/stat.h>
#include <vector>

namespace {
QString archivePageWindowTitle(const QUrl &pageUrl, const QUrl &archiveFileUrl,
    const QUrl &archiveRootUrl, KiriView::ArchiveDocumentKind kind)
{
    const KiriView::ArchiveDocumentLocation archiveDocument
        = KiriView::ArchiveDocumentLocation::fromUrls(archiveFileUrl, archiveRootUrl, kind);
    const KiriView::DisplayedImageLocation location
        = KiriView::DisplayedImageLocation::fromArchiveDocument(pageUrl, archiveDocument);
    return KiriView::windowTitleFileNameForDisplayedLocation(location);
}
}

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
    void containerCandidatesOnlyIncludeComicBookArchives();
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

    const std::optional<QUrl> cbrRootUrl
        = KiriView::comicBookArchiveRootUrl(QUrl::fromLocalFile(QStringLiteral("/books/book.cbr")));
    QVERIFY(cbrRootUrl.has_value());
    QCOMPARE(cbrRootUrl->scheme(), QStringLiteral("rar"));
    QCOMPARE(cbrRootUrl->path(), QStringLiteral("/books/book.cbr/"));
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

    const std::optional<QUrl> rarRootUrl = KiriView::directArchiveOpenRootUrl(
        QUrl::fromLocalFile(QStringLiteral("/books/book.rar")));
    QVERIFY(rarRootUrl.has_value());
    QCOMPARE(rarRootUrl->scheme(), QStringLiteral("rar"));
    QCOMPARE(rarRootUrl->path(), QStringLiteral("/books/book.rar/"));
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
    const QString title = archivePageWindowTitle(
        pageUrl, archiveUrl, *archiveRootUrl, KiriView::ArchiveDocumentKind::ComicBook);
    QCOMPARE(title, QStringLiteral("book.cbz"));
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
    const QString title = archivePageWindowTitle(
        pageUrl, archiveUrl, *archiveRootUrl, KiriView::ArchiveDocumentKind::General);
    QCOMPARE(title, QStringLiteral("book.zip"));
}

void TestImageContainer::cbtAndCb7InteriorUrlsResolveToTheirRoots()
{
    const QUrl cbtRootUrl(QStringLiteral("tar:///books/book.cbt/"));
    QUrl cbtPageUrl = cbtRootUrl;
    cbtPageUrl.setPath(cbtRootUrl.path() + QStringLiteral("chapter/page001.png"));
    QCOMPARE(KiriView::containingComicBookArchiveRootUrl(cbtPageUrl), cbtRootUrl);
    QCOMPARE(
        archivePageWindowTitle(cbtPageUrl, QUrl::fromLocalFile(QStringLiteral("/books/book.cbt")),
            cbtRootUrl, KiriView::ArchiveDocumentKind::ComicBook),
        QStringLiteral("book.cbt"));

    const QUrl cb7RootUrl(QStringLiteral("sevenz:///books/book.cb7/"));
    QUrl cb7PageUrl = cb7RootUrl;
    cb7PageUrl.setPath(cb7RootUrl.path() + QStringLiteral("chapter/page001.png"));
    QCOMPARE(KiriView::containingComicBookArchiveRootUrl(cb7PageUrl), cb7RootUrl);
    QCOMPARE(
        archivePageWindowTitle(cb7PageUrl, QUrl::fromLocalFile(QStringLiteral("/books/book.cb7")),
            cb7RootUrl, KiriView::ArchiveDocumentKind::ComicBook),
        QStringLiteral("book.cb7"));

    const QUrl cbrRootUrl(QStringLiteral("rar:///books/book.cbr/"));
    QUrl cbrPageUrl = cbrRootUrl;
    cbrPageUrl.setPath(cbrRootUrl.path() + QStringLiteral("chapter/page001.png"));
    QCOMPARE(KiriView::containingComicBookArchiveRootUrl(cbrPageUrl), cbrRootUrl);
    QCOMPARE(
        archivePageWindowTitle(cbrPageUrl, QUrl::fromLocalFile(QStringLiteral("/books/book.cbr")),
            cbrRootUrl, KiriView::ArchiveDocumentKind::ComicBook),
        QStringLiteral("book.cbr"));

    const QUrl rarRootUrl(QStringLiteral("rar:///books/book.rar/"));
    QUrl rarPageUrl = rarRootUrl;
    rarPageUrl.setPath(rarRootUrl.path() + QStringLiteral("chapter/page001.png"));
    QCOMPARE(KiriView::containingDirectArchiveOpenRootUrl(rarPageUrl), rarRootUrl);
    QCOMPARE(
        archivePageWindowTitle(rarPageUrl, QUrl::fromLocalFile(QStringLiteral("/books/book.rar")),
            rarRootUrl, KiriView::ArchiveDocumentKind::General),
        QStringLiteral("book.rar"));
}

void TestImageContainer::archiveImageContainerUrlsResolveToArchiveFile()
{
    const QUrl archiveUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());

    QUrl pageUrl = archiveDocument->rootUrl();
    pageUrl.setPath(archiveDocument->rootUrl().path() + QStringLiteral("chapter/page001.png"));
    const KiriView::DisplayedImageLocation location
        = KiriView::DisplayedImageLocation::fromArchiveDocument(pageUrl, *archiveDocument);

    QCOMPARE(KiriView::containerNavigationUrlForLocation(location), archiveUrl);
    QCOMPARE(KiriView::imageContainerUrlForLocation(location), archiveUrl);
}

void TestImageContainer::directArchivePagesDoNotResolveToContainerNavigationUrls()
{
    const QUrl archiveUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.zip"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());

    QUrl pageUrl = archiveDocument->rootUrl();
    pageUrl.setPath(archiveDocument->rootUrl().path() + QStringLiteral("chapter/page001.png"));
    const KiriView::DisplayedImageLocation location
        = KiriView::DisplayedImageLocation::fromArchiveDocument(pageUrl, *archiveDocument);

    QVERIFY(KiriView::containerNavigationUrlForLocation(location).isEmpty());
    QCOMPARE(KiriView::imageContainerUrlForLocation(location), archiveUrl);
}

void TestImageContainer::regularImageContainerUrlsResolveToParentDirectory()
{
    const QUrl fileUrl = QUrl::fromLocalFile(QStringLiteral("/images/page.png"));
    const QUrl parentUrl = QUrl::fromLocalFile(QStringLiteral("/images/"));
    const KiriView::DisplayedImageLocation location
        = KiriView::DisplayedImageLocation::fromUrl(fileUrl);

    QVERIFY(KiriView::sameContainerNavigationUrl(
        KiriView::imageContainerUrlForLocation(location), parentUrl));
    QVERIFY(KiriView::containerNavigationUrlForLocation(location).isEmpty());
}

void TestImageContainer::containerCandidatesOnlyIncludeComicBookArchives()
{
    KFileItemList items;
    items.append(KFileItem(QUrl::fromLocalFile(QStringLiteral("/books/a/")), QString(), S_IFDIR));
    items.append(
        KFileItem(QUrl::fromLocalFile(QStringLiteral("/books/a.cbz")), QString(), S_IFREG));
    items.append(
        KFileItem(QUrl::fromLocalFile(QStringLiteral("/books/b.cbr")), QString(), S_IFREG));
    items.append(
        KFileItem(QUrl::fromLocalFile(QStringLiteral("/books/book.zip")), QString(), S_IFREG));
    items.append(
        KFileItem(QUrl::fromLocalFile(QStringLiteral("/books/book.rar")), QString(), S_IFREG));

    const std::vector<KiriView::ContainerNavigationCandidate> candidates
        = KiriView::containerNavigationCandidates(items);
    QCOMPARE(candidates.size(), std::size_t(2));
    QCOMPARE(candidates.front().url, QUrl::fromLocalFile(QStringLiteral("/books/a.cbz")));
    QCOMPARE(candidates.front().type, KiriView::ContainerNavigationCandidateType::ComicBookArchive);
    QCOMPARE(candidates.back().url, QUrl::fromLocalFile(QStringLiteral("/books/b.cbr")));
    QCOMPARE(candidates.back().type, KiriView::ContainerNavigationCandidateType::ComicBookArchive);
}

QTEST_GUILESS_MAIN(TestImageContainer)

#include "test_imagecontainer.moc"
