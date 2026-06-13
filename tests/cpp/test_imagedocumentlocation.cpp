// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "location/imagedocumentlocation.h"

#include <QObject>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <optional>
#include <vector>

namespace {
QString archivePageWindowTitle(const QUrl &pageUrl, const QUrl &archiveFileUrl,
    const QUrl &archiveRootUrl, kiriview::OpenedCollectionScopeKind kind)
{
    const kiriview::OpenedCollectionScopeLocation archiveCollection
        = kiriview::OpenedCollectionScopeLocation::fromUrls(archiveFileUrl, archiveRootUrl, kind);
    const kiriview::DisplayedImageLocation location
        = kiriview::DisplayedImageLocation::fromOpenedCollectionScope(pageUrl, archiveCollection);
    return kiriview::windowTitleFileNameForDisplayedLocation(location);
}

struct ArchiveInteriorCase {
    QString scheme;
    QString extension;
    kiriview::OpenedCollectionScopeKind kind;
};

QUrl archiveRootUrl(const QString &scheme, const QString &archiveName)
{
    QUrl rootUrl;
    rootUrl.setScheme(scheme);
    rootUrl.setPath(QStringLiteral("/books/") + archiveName + QLatin1Char('/'));
    return rootUrl;
}

std::optional<QUrl> containingArchiveRootUrl(
    const QUrl &pageUrl, kiriview::OpenedCollectionScopeKind kind)
{
    if (kind == kiriview::OpenedCollectionScopeKind::ComicBookArchive) {
        return kiriview::containingComicBookArchiveRootUrl(pageUrl);
    }

    return kiriview::containingDirectArchiveOpenRootUrl(pageUrl);
}

void verifyArchiveInteriorRoot(const ArchiveInteriorCase &testCase)
{
    const QString archiveName = QStringLiteral("book.") + testCase.extension;
    const QUrl archiveFileUrl = QUrl::fromLocalFile(QStringLiteral("/books/") + archiveName);
    const QUrl rootUrl = archiveRootUrl(testCase.scheme, archiveName);
    QUrl pageUrl = rootUrl;
    pageUrl.setPath(rootUrl.path() + QStringLiteral("chapter/page001.png"));

    QVERIFY(kiriview::isUrlInsideArchiveRoot(pageUrl, rootUrl));

    const std::optional<QUrl> containingRoot = containingArchiveRootUrl(pageUrl, testCase.kind);
    QVERIFY(containingRoot.has_value());
    QCOMPARE(*containingRoot, rootUrl);
    QCOMPARE(archivePageWindowTitle(pageUrl, archiveFileUrl, rootUrl, testCase.kind), archiveName);
}
}

class TestImageDocumentLocation : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void comicBookArchiveRootUrlsUseZipScheme();
    void comicBookArchiveRootUrlsUseFormatSpecificKioSchemes();
    void directArchiveRootUrlsUseFormatSpecificKioSchemes();
    void archiveInteriorUrlsResolveToTheirRootAndTitle();
    void archiveCollectionPagesResolveToArchiveZoomScope();
    void directArchivePagesResolveToZoomScopeOnly();
    void directoryCollectionPagesResolveToDirectoryCollectionScope();
    void regularImagesDoNotResolveToZoomScopes();
    void explicitKdeArchiveUrlImagesDoNotResolveToZoomScopes();
};

void TestImageDocumentLocation::comicBookArchiveRootUrlsUseZipScheme()
{
    const QUrl archiveUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = kiriview::comicBookArchiveRootUrl(archiveUrl);

    QVERIFY(archiveRootUrl.has_value());
    QCOMPARE(archiveRootUrl->scheme(), QStringLiteral("zip"));
    QCOMPARE(archiveRootUrl->path(), QStringLiteral("/books/book.cbz/"));
}

void TestImageDocumentLocation::comicBookArchiveRootUrlsUseFormatSpecificKioSchemes()
{
    const std::optional<QUrl> cbtRootUrl
        = kiriview::comicBookArchiveRootUrl(QUrl::fromLocalFile(QStringLiteral("/books/book.cbt")));
    QVERIFY(cbtRootUrl.has_value());
    QCOMPARE(cbtRootUrl->scheme(), QStringLiteral("tar"));
    QCOMPARE(cbtRootUrl->path(), QStringLiteral("/books/book.cbt/"));

    const std::optional<QUrl> cb7RootUrl
        = kiriview::comicBookArchiveRootUrl(QUrl::fromLocalFile(QStringLiteral("/books/book.cb7")));
    QVERIFY(cb7RootUrl.has_value());
    QCOMPARE(cb7RootUrl->scheme(), QStringLiteral("sevenz"));
    QCOMPARE(cb7RootUrl->path(), QStringLiteral("/books/book.cb7/"));

    const std::optional<QUrl> cbrRootUrl
        = kiriview::comicBookArchiveRootUrl(QUrl::fromLocalFile(QStringLiteral("/books/book.cbr")));
    QVERIFY(cbrRootUrl.has_value());
    QCOMPARE(cbrRootUrl->scheme(), QStringLiteral("rar"));
    QCOMPARE(cbrRootUrl->path(), QStringLiteral("/books/book.cbr/"));

    QVERIFY(
        !kiriview::comicBookArchiveRootUrl(QUrl::fromLocalFile(QStringLiteral("/books/book.zip")))
            .has_value());
    QVERIFY(!kiriview::comicBookArchiveRootUrl(QUrl(QStringLiteral("smb://server/books/book.cbz")))
            .has_value());
}

void TestImageDocumentLocation::directArchiveRootUrlsUseFormatSpecificKioSchemes()
{
    const std::optional<QUrl> zipRootUrl = kiriview::directArchiveOpenRootUrl(
        QUrl::fromLocalFile(QStringLiteral("/books/book.zip")));
    QVERIFY(zipRootUrl.has_value());
    QCOMPARE(zipRootUrl->scheme(), QStringLiteral("zip"));
    QCOMPARE(zipRootUrl->path(), QStringLiteral("/books/book.zip/"));

    const std::optional<QUrl> tarRootUrl = kiriview::directArchiveOpenRootUrl(
        QUrl::fromLocalFile(QStringLiteral("/books/book.tar")));
    QVERIFY(tarRootUrl.has_value());
    QCOMPARE(tarRootUrl->scheme(), QStringLiteral("tar"));
    QCOMPARE(tarRootUrl->path(), QStringLiteral("/books/book.tar/"));

    const std::optional<QUrl> sevenZipRootUrl
        = kiriview::directArchiveOpenRootUrl(QUrl::fromLocalFile(QStringLiteral("/books/book.7z")));
    QVERIFY(sevenZipRootUrl.has_value());
    QCOMPARE(sevenZipRootUrl->scheme(), QStringLiteral("sevenz"));
    QCOMPARE(sevenZipRootUrl->path(), QStringLiteral("/books/book.7z/"));

    const std::optional<QUrl> rarRootUrl = kiriview::directArchiveOpenRootUrl(
        QUrl::fromLocalFile(QStringLiteral("/books/book.rar")));
    QVERIFY(rarRootUrl.has_value());
    QCOMPARE(rarRootUrl->scheme(), QStringLiteral("rar"));
    QCOMPARE(rarRootUrl->path(), QStringLiteral("/books/book.rar/"));

    QVERIFY(!kiriview::directArchiveOpenRootUrl(QUrl(QStringLiteral("smb://server/books/book.zip")))
            .has_value());
}

void TestImageDocumentLocation::archiveInteriorUrlsResolveToTheirRootAndTitle()
{
    const std::vector<ArchiveInteriorCase> cases = {
        { QStringLiteral("zip"), QStringLiteral("cbz"),
            kiriview::OpenedCollectionScopeKind::ComicBookArchive },
        { QStringLiteral("tar"), QStringLiteral("cbt"),
            kiriview::OpenedCollectionScopeKind::ComicBookArchive },
        { QStringLiteral("sevenz"), QStringLiteral("cb7"),
            kiriview::OpenedCollectionScopeKind::ComicBookArchive },
        { QStringLiteral("rar"), QStringLiteral("cbr"),
            kiriview::OpenedCollectionScopeKind::ComicBookArchive },
        { QStringLiteral("zip"), QStringLiteral("zip"),
            kiriview::OpenedCollectionScopeKind::GeneralArchive },
        { QStringLiteral("tar"), QStringLiteral("tar"),
            kiriview::OpenedCollectionScopeKind::GeneralArchive },
        { QStringLiteral("sevenz"), QStringLiteral("7z"),
            kiriview::OpenedCollectionScopeKind::GeneralArchive },
        { QStringLiteral("rar"), QStringLiteral("rar"),
            kiriview::OpenedCollectionScopeKind::GeneralArchive },
    };

    for (const ArchiveInteriorCase &testCase : cases) {
        verifyArchiveInteriorRoot(testCase);
    }
}

void TestImageDocumentLocation::archiveCollectionPagesResolveToArchiveZoomScope()
{
    const QUrl archiveUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());

    QUrl pageUrl = archiveCollection->rootUrl();
    pageUrl.setPath(archiveCollection->rootUrl().path() + QStringLiteral("chapter/page001.png"));
    const kiriview::DisplayedImageLocation location
        = kiriview::DisplayedImageLocation::fromOpenedCollectionScope(pageUrl, *archiveCollection);

    QCOMPARE(kiriview::containerNavigationUrlForLocation(location), archiveUrl);
    QCOMPARE(kiriview::zoomScopeUrlForLocation(location), archiveUrl);
}

void TestImageDocumentLocation::directArchivePagesResolveToZoomScopeOnly()
{
    const QUrl archiveUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.zip"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());

    QUrl pageUrl = archiveCollection->rootUrl();
    pageUrl.setPath(archiveCollection->rootUrl().path() + QStringLiteral("chapter/page001.png"));
    const kiriview::DisplayedImageLocation location
        = kiriview::DisplayedImageLocation::fromOpenedCollectionScope(pageUrl, *archiveCollection);

    QVERIFY(kiriview::containerNavigationUrlForLocation(location).isEmpty());
    QCOMPARE(kiriview::zoomScopeUrlForLocation(location), archiveUrl);
}

void TestImageDocumentLocation::directoryCollectionPagesResolveToDirectoryCollectionScope()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QUrl directoryUrl = QUrl::fromLocalFile(directory.path());
    const std::optional<kiriview::OpenedCollectionScopeLocation> directoryCollection
        = kiriview::openedCollectionScopeLocationForDirectlyOpenedLocalUrl(directoryUrl);
    QVERIFY(directoryCollection.has_value());
    QCOMPARE(directoryCollection->kind(), kiriview::OpenedCollectionScopeKind::Directory);
    QCOMPARE(directoryCollection->fileUrl(), directoryUrl);
    QCOMPARE(directoryCollection->rootUrl().scheme(), QStringLiteral("file"));
    QVERIFY(directoryCollection->rootUrl().path().endsWith(QLatin1Char('/')));

    QUrl pageUrl = directoryCollection->rootUrl();
    pageUrl.setPath(directoryCollection->rootUrl().path() + QStringLiteral("chapter/page001.png"));
    const kiriview::DisplayedImageLocation location
        = kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
            pageUrl, *directoryCollection);

    QVERIFY(kiriview::containerNavigationUrlForLocation(location).isEmpty());
    QCOMPARE(kiriview::zoomScopeUrlForLocation(location), directoryUrl);
    QCOMPARE(kiriview::windowTitleFileNameForDisplayedLocation(location), directoryUrl.fileName());
}

void TestImageDocumentLocation::regularImagesDoNotResolveToZoomScopes()
{
    const QUrl fileUrl = QUrl::fromLocalFile(QStringLiteral("/images/page.png"));
    const kiriview::DisplayedImageLocation location
        = kiriview::DisplayedImageLocation::fromUrl(fileUrl);

    QVERIFY(kiriview::zoomScopeUrlForLocation(location).isEmpty());
    QVERIFY(kiriview::containerNavigationUrlForLocation(location).isEmpty());
}

void TestImageDocumentLocation::explicitKdeArchiveUrlImagesDoNotResolveToZoomScopes()
{
    const kiriview::DisplayedImageLocation location = kiriview::DisplayedImageLocation::fromUrl(
        QUrl(QStringLiteral("zip:///books/book.cbz/chapter/page001.png")));

    QVERIFY(kiriview::zoomScopeUrlForLocation(location).isEmpty());
    QVERIFY(kiriview::containerNavigationUrlForLocation(location).isEmpty());
}

QTEST_GUILESS_MAIN(TestImageDocumentLocation)

#include "test_imagedocumentlocation.moc"
