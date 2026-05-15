// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainer.h"

#include <QObject>
#include <QTemporaryDir>
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

struct ArchiveInteriorCase {
    QString scheme;
    QString extension;
    KiriView::ArchiveDocumentKind kind;
};

QUrl archiveRootUrl(const QString &scheme, const QString &archiveName)
{
    QUrl rootUrl;
    rootUrl.setScheme(scheme);
    rootUrl.setPath(QStringLiteral("/books/") + archiveName + QLatin1Char('/'));
    return rootUrl;
}

std::optional<QUrl> containingArchiveRootUrl(
    const QUrl &pageUrl, KiriView::ArchiveDocumentKind kind)
{
    if (kind == KiriView::ArchiveDocumentKind::ComicBook) {
        return KiriView::containingComicBookArchiveRootUrl(pageUrl);
    }

    return KiriView::containingDirectArchiveOpenRootUrl(pageUrl);
}

void verifyArchiveInteriorRoot(const ArchiveInteriorCase &testCase)
{
    const QString archiveName = QStringLiteral("book.") + testCase.extension;
    const QUrl archiveFileUrl = QUrl::fromLocalFile(QStringLiteral("/books/") + archiveName);
    const QUrl rootUrl = archiveRootUrl(testCase.scheme, archiveName);
    QUrl pageUrl = rootUrl;
    pageUrl.setPath(rootUrl.path() + QStringLiteral("chapter/page001.png"));

    QVERIFY(KiriView::isUrlInsideArchiveRoot(pageUrl, rootUrl));

    const std::optional<QUrl> containingRoot = containingArchiveRootUrl(pageUrl, testCase.kind);
    QVERIFY(containingRoot.has_value());
    QCOMPARE(*containingRoot, rootUrl);
    QCOMPARE(archivePageWindowTitle(pageUrl, archiveFileUrl, rootUrl, testCase.kind), archiveName);
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
    void archiveDocumentPagesResolveToArchiveZoomScope();
    void directArchivePagesResolveToZoomScopeOnly();
    void directDirectoryPagesResolveToDirectoryDocumentScope();
    void regularImagesDoNotResolveToZoomScopes();
    void explicitKdeArchiveUrlImagesDoNotResolveToZoomScopes();
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
    const std::vector<ArchiveInteriorCase> cases = {
        { QStringLiteral("zip"), QStringLiteral("cbz"), KiriView::ArchiveDocumentKind::ComicBook },
        { QStringLiteral("tar"), QStringLiteral("cbt"), KiriView::ArchiveDocumentKind::ComicBook },
        { QStringLiteral("sevenz"), QStringLiteral("cb7"),
            KiriView::ArchiveDocumentKind::ComicBook },
        { QStringLiteral("rar"), QStringLiteral("cbr"), KiriView::ArchiveDocumentKind::ComicBook },
        { QStringLiteral("zip"), QStringLiteral("zip"), KiriView::ArchiveDocumentKind::General },
        { QStringLiteral("tar"), QStringLiteral("tar"), KiriView::ArchiveDocumentKind::General },
        { QStringLiteral("sevenz"), QStringLiteral("7z"), KiriView::ArchiveDocumentKind::General },
        { QStringLiteral("rar"), QStringLiteral("rar"), KiriView::ArchiveDocumentKind::General },
    };

    for (const ArchiveInteriorCase &testCase : cases) {
        verifyArchiveInteriorRoot(testCase);
    }
}

void TestImageContainer::archiveDocumentPagesResolveToArchiveZoomScope()
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
    QCOMPARE(KiriView::zoomScopeUrlForLocation(location), archiveUrl);
}

void TestImageContainer::directArchivePagesResolveToZoomScopeOnly()
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
    QCOMPARE(KiriView::zoomScopeUrlForLocation(location), archiveUrl);
}

void TestImageContainer::directDirectoryPagesResolveToDirectoryDocumentScope()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QUrl directoryUrl = QUrl::fromLocalFile(directory.path());
    const std::optional<KiriView::ArchiveDocumentLocation> directoryDocument
        = KiriView::directOpenDocumentLocationForLocalUrl(directoryUrl);
    QVERIFY(directoryDocument.has_value());
    QCOMPARE(directoryDocument->kind(), KiriView::ArchiveDocumentKind::Directory);
    QCOMPARE(directoryDocument->fileUrl(), directoryUrl);
    QCOMPARE(directoryDocument->rootUrl().scheme(), QStringLiteral("file"));
    QVERIFY(directoryDocument->rootUrl().path().endsWith(QLatin1Char('/')));

    QUrl pageUrl = directoryDocument->rootUrl();
    pageUrl.setPath(directoryDocument->rootUrl().path() + QStringLiteral("chapter/page001.png"));
    const KiriView::DisplayedImageLocation location
        = KiriView::DisplayedImageLocation::fromArchiveDocument(pageUrl, *directoryDocument);

    QVERIFY(KiriView::containerNavigationUrlForLocation(location).isEmpty());
    QCOMPARE(KiriView::zoomScopeUrlForLocation(location), directoryUrl);
    QCOMPARE(KiriView::windowTitleFileNameForDisplayedLocation(location), directoryUrl.fileName());
}

void TestImageContainer::regularImagesDoNotResolveToZoomScopes()
{
    const QUrl fileUrl = QUrl::fromLocalFile(QStringLiteral("/images/page.png"));
    const KiriView::DisplayedImageLocation location
        = KiriView::DisplayedImageLocation::fromUrl(fileUrl);

    QVERIFY(KiriView::zoomScopeUrlForLocation(location).isEmpty());
    QVERIFY(KiriView::containerNavigationUrlForLocation(location).isEmpty());
}

void TestImageContainer::explicitKdeArchiveUrlImagesDoNotResolveToZoomScopes()
{
    const KiriView::DisplayedImageLocation location = KiriView::DisplayedImageLocation::fromUrl(
        QUrl(QStringLiteral("zip:///books/book.cbz/chapter/page001.png")));

    QVERIFY(KiriView::zoomScopeUrlForLocation(location).isEmpty());
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
