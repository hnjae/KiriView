// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imageloadtypes.h"
#include "location/imagelocation.h"
#include "location/imageurl.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QtGlobal>
#include <cstddef>
#include <optional>
#include <sys/xattr.h>

namespace {
QUrl archiveUrl(const QString& scheme, const QString& path)
{
    QUrl url;
    url.setScheme(scheme);
    url.setPath(path);
    return url;
}

bool writeEmptyFile(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly);
}
}

class TestImageUrl : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void normalizedContainerUrlsStripQueryFragmentsAndCleanLocalPaths();
    void normalizedUrlIdentityHelpersRejectInvalidImageUrlsAndPreserveKeyFormatting();
    void directoryNavigationHelpersOwnParentAndIdentityRules();
    void normalizedUrlIdentityComparisonHandlesEmptyAndPathEquivalentUrls();
    void sameNormalizedUrlMatchesPathSegments();
    void sameContainerNavigationUrlMatchesNormalizedPaths();
    void parentUrlForContainerNavigationHandlesContainers();
    void navigationSourceFactsResolveDocumentPortalHostWithoutXattr();
    void navigationSourceFactsRestoreKioFuseArchivesWithoutEnvironment();
    void documentPortalHostPathOwnsNavigationScope();
    void kioFuseArchivePathsRestoreSupportedArchiveSchemes();
    void imageLocationTypesExposeExplicitState();
};

void TestImageUrl::normalizedContainerUrlsStripQueryFragmentsAndCleanLocalPaths()
{
    QUrl fileUrl = QUrl::fromLocalFile(QStringLiteral("/images/./chapter/../page.png"));
    fileUrl.setQuery(QStringLiteral("cache=1"));
    fileUrl.setFragment(QStringLiteral("view"));
    QCOMPARE(kiriview::normalizedFileContainerUrl(fileUrl),
        QUrl::fromLocalFile(QStringLiteral("/images/page.png")));

    QUrl directoryUrl = QUrl::fromLocalFile(QStringLiteral("/images/chapter"));
    directoryUrl.setQuery(QStringLiteral("cache=1"));
    directoryUrl.setFragment(QStringLiteral("view"));
    QCOMPARE(kiriview::normalizedDirectoryContainerUrl(directoryUrl),
        QUrl::fromLocalFile(QStringLiteral("/images/chapter/")));
}

void TestImageUrl::normalizedUrlIdentityHelpersRejectInvalidImageUrlsAndPreserveKeyFormatting()
{
    const QUrl normalizedUrl = QUrl::fromLocalFile(QStringLiteral("/images/page 1.png"));
    const QUrl equivalentUrl = QUrl::fromLocalFile(QStringLiteral("/images/chapter/../page 1.png"));

    QCOMPARE(kiriview::normalizedUrlForIdentity(equivalentUrl), normalizedUrl);
    const std::optional<QUrl> validImageUrl = kiriview::normalizedValidImageUrl(equivalentUrl);
    QVERIFY(validImageUrl.has_value());
    QCOMPARE(*validImageUrl, normalizedUrl);
    QVERIFY(!kiriview::normalizedValidImageUrl(QUrl()).has_value());
    QCOMPARE(kiriview::normalizedUrlIdentityKey(equivalentUrl),
        normalizedUrl.toString(QUrl::PrettyDecoded));
    QCOMPARE(kiriview::normalizedUrlIdentityKey(equivalentUrl, QUrl::FullyEncoded),
        normalizedUrl.toString(QUrl::FullyEncoded));
}

void TestImageUrl::directoryNavigationHelpersOwnParentAndIdentityRules()
{
    const QUrl directoryUrl = QUrl::fromLocalFile(QStringLiteral("/images/chapter/../"));
    const QUrl normalizedDirectoryUrl = QUrl::fromLocalFile(QStringLiteral("/images/"));
    QCOMPARE(kiriview::normalizedDirectoryUrlForIdentity(directoryUrl), normalizedDirectoryUrl);
    QCOMPARE(kiriview::directoryUrlIdentityKey(directoryUrl),
        normalizedDirectoryUrl.toString(QUrl::FullyEncoded));

    QCOMPARE(kiriview::parentDirectoryUrlForFileNavigation(
                 QUrl::fromLocalFile(QStringLiteral("/images/a/../b/page.png"))),
        QUrl::fromLocalFile(QStringLiteral("/images/b/")));

    const QUrl archiveEntry(QStringLiteral("zip:///path/archive.zip!/chapter/page.png"));
    QCOMPARE(kiriview::parentDirectoryUrlForFileNavigation(archiveEntry),
        QUrl(QStringLiteral("zip:///path/archive.zip!/chapter/")));

    const kiriview::DirectoryNavigationLocation navigationLocation
        = kiriview::directoryNavigationLocationForFileUrl(
            QUrl::fromLocalFile(QStringLiteral("/images/a/../b/page.png")));
    QVERIFY(navigationLocation.isValid());
    QVERIFY(kiriview::sameNormalizedUrl(
        navigationLocation.fileUrl, QUrl::fromLocalFile(QStringLiteral("/images/b/page.png"))));
    QCOMPARE(navigationLocation.directoryUrl, QUrl::fromLocalFile(QStringLiteral("/images/b/")));
}

void TestImageUrl::normalizedUrlIdentityComparisonHandlesEmptyAndPathEquivalentUrls()
{
    const QUrl rawUrl = QUrl::fromLocalFile(QStringLiteral("/images/chapter/../page.png"));
    const QUrl normalizedUrl = QUrl::fromLocalFile(QStringLiteral("/images/page.png"));

    const std::optional<QUrl> validUrl = kiriview::normalizedValidUrlForIdentity(rawUrl);
    QVERIFY(validUrl.has_value());
    QCOMPARE(*validUrl, normalizedUrl);
    QVERIFY(!kiriview::normalizedValidUrlForIdentity(QUrl()).has_value());
    QVERIFY(kiriview::sameNormalizedUrlOrEmpty(rawUrl, normalizedUrl));
    QVERIFY(kiriview::sameNormalizedUrlOrEmpty(QUrl(), QUrl()));
    QVERIFY(!kiriview::sameNormalizedUrlOrEmpty(rawUrl, QUrl()));
}

void TestImageUrl::sameNormalizedUrlMatchesPathSegments()
{
    QVERIFY(kiriview::sameNormalizedUrl(
        QUrl::fromLocalFile(QStringLiteral("/images/chapter/../page.png")),
        QUrl::fromLocalFile(QStringLiteral("/images/page.png"))));
    QVERIFY(!kiriview::sameNormalizedUrl(QUrl::fromLocalFile(QStringLiteral("/images/page.png")),
        QUrl::fromLocalFile(QStringLiteral("/images/other.png"))));
}

void TestImageUrl::sameContainerNavigationUrlMatchesNormalizedPaths()
{
    QVERIFY(kiriview::sameContainerNavigationUrl(
        QUrl::fromLocalFile(QStringLiteral("/images/chapter/../")),
        QUrl::fromLocalFile(QStringLiteral("/images/"))));
    QVERIFY(!kiriview::sameContainerNavigationUrl(
        QUrl(), QUrl::fromLocalFile(QStringLiteral("/images/"))));
}

void TestImageUrl::parentUrlForContainerNavigationHandlesContainers()
{
    QVERIFY(kiriview::sameContainerNavigationUrl(
        kiriview::parentUrlForContainerNavigation(QUrl::fromLocalFile(QStringLiteral("/images/"))),
        QUrl::fromLocalFile(QStringLiteral("/"))));
    const QUrl archiveUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbz"));
    QCOMPARE(kiriview::parentUrlForContainerNavigation(archiveUrl),
        QUrl::fromLocalFile(QStringLiteral("/books/")));
}

void TestImageUrl::navigationSourceFactsResolveDocumentPortalHostWithoutXattr()
{
    const QUrl portalUrl = QUrl::fromLocalFile(QStringLiteral("/run/user/1000/doc/02.mp4"));
    const QUrl hostUrl = QUrl::fromLocalFile(QStringLiteral("/media/videos/02.mp4"));

    kiriview::NavigationSourceFacts facts;
    facts.documentPortalHostPath = hostUrl.toLocalFile();
    QCOMPARE(kiriview::navigationSourceUrlForFacts(portalUrl, facts), hostUrl);

    facts.documentPortalHostPath = portalUrl.toLocalFile();
    QCOMPARE(kiriview::navigationSourceUrlForFacts(portalUrl, facts), portalUrl);
}

void TestImageUrl::navigationSourceFactsRestoreKioFuseArchivesWithoutEnvironment()
{
    kiriview::NavigationSourceFacts facts;
    facts.runtimeDir = QStringLiteral("/run/user/1000");

    const QString cbzFusePath
        = QStringLiteral("/run/user/1000/kio-fuse-test/zip/books/book.cbz/page.png");
    const QString cbtFusePath
        = QStringLiteral("/run/user/1000/kio-fuse-test/tar/books/book.cbt/page.png");
    const QString cb7FusePath
        = QStringLiteral("/run/user/1000/kio-fuse-test/sevenz/books/book.cb7/page.png");

    QCOMPARE(kiriview::navigationSourceUrlForFacts(QUrl::fromLocalFile(cbzFusePath), facts),
        archiveUrl(QStringLiteral("zip"), QStringLiteral("/books/book.cbz/page.png")));
    QCOMPARE(kiriview::navigationSourceUrlForFacts(QUrl::fromLocalFile(cbtFusePath), facts),
        archiveUrl(QStringLiteral("tar"), QStringLiteral("/books/book.cbt/page.png")));
    QCOMPARE(kiriview::navigationSourceUrlForFacts(QUrl::fromLocalFile(cb7FusePath), facts),
        archiveUrl(QStringLiteral("sevenz"), QStringLiteral("/books/book.cb7/page.png")));
}

void TestImageUrl::documentPortalHostPathOwnsNavigationScope()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("portal"))));
    QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("host"))));

    const QString portalPath = directory.filePath(QStringLiteral("portal/02.mp4"));
    const QString hostPath = directory.filePath(QStringLiteral("host/02.mp4"));
    QVERIFY(writeEmptyFile(portalPath));
    QVERIFY(writeEmptyFile(hostPath));

    const QByteArray encodedPortalPath = QFile::encodeName(portalPath);
    const QByteArray encodedHostPath = QFile::encodeName(hostPath);
    const int result = setxattr(encodedPortalPath.constData(), "user.document-portal.host-path",
        encodedHostPath.constData(), static_cast<std::size_t>(encodedHostPath.size()), 0);
    if (result != 0) {
        QSKIP("extended attributes unavailable");
    }

    const QUrl portalUrl = QUrl::fromLocalFile(portalPath);
    const QUrl hostUrl = QUrl::fromLocalFile(hostPath);
    QCOMPARE(kiriview::navigationSourceUrl(portalUrl), hostUrl);

    const kiriview::DirectoryNavigationLocation navigationLocation
        = kiriview::directoryNavigationLocationForFileUrl(portalUrl);
    QCOMPARE(navigationLocation.fileUrl, hostUrl);
    QCOMPARE(navigationLocation.directoryUrl,
        QUrl::fromLocalFile(directory.filePath(QStringLiteral("host/"))));
}

void TestImageUrl::kioFuseArchivePathsRestoreSupportedArchiveSchemes()
{
    const bool hadRuntimeDir = qEnvironmentVariableIsSet("XDG_RUNTIME_DIR");
    const QByteArray originalRuntimeDir = qgetenv("XDG_RUNTIME_DIR");
    qputenv("XDG_RUNTIME_DIR", "/run/user/1000");

    const QString cbzFusePath
        = QStringLiteral("/run/user/1000/kio-fuse-test/zip/books/book.cbz/page.png");
    const QString cbtFusePath
        = QStringLiteral("/run/user/1000/kio-fuse-test/tar/books/book.cbt/page.png");
    const QString cb7FusePath
        = QStringLiteral("/run/user/1000/kio-fuse-test/sevenz/books/book.cb7/page.png");

    QCOMPARE(kiriview::navigationSourceUrl(QUrl::fromLocalFile(cbzFusePath)),
        archiveUrl(QStringLiteral("zip"), QStringLiteral("/books/book.cbz/page.png")));
    QCOMPARE(kiriview::navigationSourceUrl(QUrl::fromLocalFile(cbtFusePath)),
        archiveUrl(QStringLiteral("tar"), QStringLiteral("/books/book.cbt/page.png")));
    QCOMPARE(kiriview::navigationSourceUrl(QUrl::fromLocalFile(cb7FusePath)),
        archiveUrl(QStringLiteral("sevenz"), QStringLiteral("/books/book.cb7/page.png")));

    if (hadRuntimeDir) {
        qputenv("XDG_RUNTIME_DIR", originalRuntimeDir);
    } else {
        qunsetenv("XDG_RUNTIME_DIR");
    }
}

void TestImageUrl::imageLocationTypesExposeExplicitState()
{
    const kiriview::DisplayedImageLocation emptyLocation;
    QVERIFY(emptyLocation.isEmpty());

    const kiriview::OpenedCollectionScopeLocation archiveCollection
        = kiriview::OpenedCollectionScopeLocation::fromUrls(
            QUrl::fromLocalFile(QStringLiteral("/books/book.cbz")),
            QUrl(QStringLiteral("zip:///books/book.cbz/")),
            kiriview::OpenedCollectionScopeKind::ComicBookArchive);
    const kiriview::DisplayedImageLocation location
        = kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
            QUrl(QStringLiteral("zip:///books/book.cbz/page.png")), archiveCollection);
    QVERIFY(!location.isEmpty());
    QCOMPARE(location.openedCollectionScopeSourceUrl(), archiveCollection.fileUrl());
    QCOMPARE(location.openedCollectionScopeRootUrl(), archiveCollection.rootUrl());

    const kiriview::ImageLoadRequest plainOpen
        = kiriview::ImageLoadRequest::fromUrl(location.imageUrl());
    QVERIFY(!plainOpen.isEmpty());
    QVERIFY(!plainOpen.isContainerNavigation());
    QCOMPARE(plainOpen.sourceUrl(), location.imageUrl());
    QVERIFY(plainOpen.openedCollectionScope().rootUrl().isEmpty());

    const QUrl containerUrl = QUrl::fromLocalFile(QStringLiteral("/images/"));
    const kiriview::ImageLoadRequest containerOpen = kiriview::ImageLoadRequest::fromLocation(
        location.imageUrl(), location.openedCollectionScope(), containerUrl);
    QVERIFY(containerOpen.isContainerNavigation());
    QCOMPARE(
        containerOpen.openedCollectionScope().rootUrl(), location.openedCollectionScopeRootUrl());
    QCOMPARE(containerOpen.containerNavigationUrl(), containerUrl);
}

QTEST_GUILESS_MAIN(TestImageUrl)

#include "test_imageurl.moc"
