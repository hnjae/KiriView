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
QUrl archiveUrl(const QString &scheme, const QString &path)
{
    QUrl url;
    url.setScheme(scheme);
    url.setPath(path);
    return url;
}

bool writeEmptyFile(const QString &path)
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
    void documentPortalHostPathOwnsNavigationScope();
    void kioFuseArchivePathsRestoreSupportedArchiveSchemes();
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

void TestImageUrl::normalizedUrlIdentityHelpersRejectInvalidImageUrlsAndPreserveKeyFormatting()
{
    const QUrl normalizedUrl = QUrl::fromLocalFile(QStringLiteral("/images/page 1.png"));
    const QUrl equivalentUrl = QUrl::fromLocalFile(QStringLiteral("/images/chapter/../page 1.png"));

    QCOMPARE(KiriView::normalizedUrlForIdentity(equivalentUrl), normalizedUrl);
    const std::optional<QUrl> validImageUrl = KiriView::normalizedValidImageUrl(equivalentUrl);
    QVERIFY(validImageUrl.has_value());
    QCOMPARE(*validImageUrl, normalizedUrl);
    QVERIFY(!KiriView::normalizedValidImageUrl(QUrl()).has_value());
    QCOMPARE(KiriView::normalizedUrlIdentityKey(equivalentUrl),
        normalizedUrl.toString(QUrl::PrettyDecoded));
    QCOMPARE(KiriView::normalizedUrlIdentityKey(equivalentUrl, QUrl::FullyEncoded),
        normalizedUrl.toString(QUrl::FullyEncoded));
}

void TestImageUrl::directoryNavigationHelpersOwnParentAndIdentityRules()
{
    const QUrl directoryUrl = QUrl::fromLocalFile(QStringLiteral("/images/chapter/../"));
    const QUrl normalizedDirectoryUrl = QUrl::fromLocalFile(QStringLiteral("/images/"));
    QCOMPARE(KiriView::normalizedDirectoryUrlForIdentity(directoryUrl), normalizedDirectoryUrl);
    QCOMPARE(KiriView::directoryUrlIdentityKey(directoryUrl),
        normalizedDirectoryUrl.toString(QUrl::FullyEncoded));

    QCOMPARE(KiriView::parentDirectoryUrlForFileNavigation(
                 QUrl::fromLocalFile(QStringLiteral("/images/a/../b/page.png"))),
        QUrl::fromLocalFile(QStringLiteral("/images/b/")));

    const QUrl archiveEntry(QStringLiteral("zip:///path/archive.zip!/chapter/page.png"));
    QCOMPARE(KiriView::parentDirectoryUrlForFileNavigation(archiveEntry),
        QUrl(QStringLiteral("zip:///path/archive.zip!/chapter/")));

    const KiriView::DirectoryNavigationLocation navigationLocation
        = KiriView::directoryNavigationLocationForFileUrl(
            QUrl::fromLocalFile(QStringLiteral("/images/a/../b/page.png")));
    QVERIFY(navigationLocation.isValid());
    QVERIFY(KiriView::sameNormalizedUrl(
        navigationLocation.fileUrl, QUrl::fromLocalFile(QStringLiteral("/images/b/page.png"))));
    QCOMPARE(navigationLocation.directoryUrl, QUrl::fromLocalFile(QStringLiteral("/images/b/")));
}

void TestImageUrl::normalizedUrlIdentityComparisonHandlesEmptyAndPathEquivalentUrls()
{
    const QUrl rawUrl = QUrl::fromLocalFile(QStringLiteral("/images/chapter/../page.png"));
    const QUrl normalizedUrl = QUrl::fromLocalFile(QStringLiteral("/images/page.png"));

    const std::optional<QUrl> validUrl = KiriView::normalizedValidUrlForIdentity(rawUrl);
    QVERIFY(validUrl.has_value());
    QCOMPARE(*validUrl, normalizedUrl);
    QVERIFY(!KiriView::normalizedValidUrlForIdentity(QUrl()).has_value());
    QVERIFY(KiriView::sameNormalizedUrlOrEmpty(rawUrl, normalizedUrl));
    QVERIFY(KiriView::sameNormalizedUrlOrEmpty(QUrl(), QUrl()));
    QVERIFY(!KiriView::sameNormalizedUrlOrEmpty(rawUrl, QUrl()));
}

void TestImageUrl::sameNormalizedUrlMatchesPathSegments()
{
    QVERIFY(KiriView::sameNormalizedUrl(
        QUrl::fromLocalFile(QStringLiteral("/images/chapter/../page.png")),
        QUrl::fromLocalFile(QStringLiteral("/images/page.png"))));
    QVERIFY(!KiriView::sameNormalizedUrl(QUrl::fromLocalFile(QStringLiteral("/images/page.png")),
        QUrl::fromLocalFile(QStringLiteral("/images/other.png"))));
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
    QCOMPARE(KiriView::navigationSourceUrl(portalUrl), hostUrl);

    const KiriView::DirectoryNavigationLocation navigationLocation
        = KiriView::directoryNavigationLocationForFileUrl(portalUrl);
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

    QCOMPARE(KiriView::navigationSourceUrl(QUrl::fromLocalFile(cbzFusePath)),
        archiveUrl(QStringLiteral("zip"), QStringLiteral("/books/book.cbz/page.png")));
    QCOMPARE(KiriView::navigationSourceUrl(QUrl::fromLocalFile(cbtFusePath)),
        archiveUrl(QStringLiteral("tar"), QStringLiteral("/books/book.cbt/page.png")));
    QCOMPARE(KiriView::navigationSourceUrl(QUrl::fromLocalFile(cb7FusePath)),
        archiveUrl(QStringLiteral("sevenz"), QStringLiteral("/books/book.cb7/page.png")));

    if (hadRuntimeDir) {
        qputenv("XDG_RUNTIME_DIR", originalRuntimeDir);
    } else {
        qunsetenv("XDG_RUNTIME_DIR");
    }
}

void TestImageUrl::imageLocationTypesExposeExplicitState()
{
    const KiriView::DisplayedImageLocation emptyLocation;
    QVERIFY(emptyLocation.isEmpty());

    const KiriView::OpenedCollectionScopeLocation archiveCollection
        = KiriView::OpenedCollectionScopeLocation::fromUrls(
            QUrl::fromLocalFile(QStringLiteral("/books/book.cbz")),
            QUrl(QStringLiteral("zip:///books/book.cbz/")),
            KiriView::OpenedCollectionScopeKind::ComicBookArchive);
    const KiriView::DisplayedImageLocation location
        = KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
            QUrl(QStringLiteral("zip:///books/book.cbz/page.png")), archiveCollection);
    QVERIFY(!location.isEmpty());
    QCOMPARE(location.openedCollectionScopeSourceUrl(), archiveCollection.fileUrl());
    QCOMPARE(location.openedCollectionScopeRootUrl(), archiveCollection.rootUrl());

    const KiriView::ImageLoadRequest plainOpen
        = KiriView::ImageLoadRequest::fromUrl(location.imageUrl());
    QVERIFY(!plainOpen.isEmpty());
    QVERIFY(!plainOpen.isContainerNavigation());
    QCOMPARE(plainOpen.sourceUrl(), location.imageUrl());
    QVERIFY(plainOpen.openedCollectionScope().rootUrl().isEmpty());

    const QUrl containerUrl = QUrl::fromLocalFile(QStringLiteral("/images/"));
    const KiriView::ImageLoadRequest containerOpen = KiriView::ImageLoadRequest::fromLocation(
        location.imageUrl(), location.openedCollectionScope(), containerUrl);
    QVERIFY(containerOpen.isContainerNavigation());
    QCOMPARE(
        containerOpen.openedCollectionScope().rootUrl(), location.openedCollectionScopeRootUrl());
    QCOMPARE(containerOpen.containerNavigationUrl(), containerUrl);
}

QTEST_GUILESS_MAIN(TestImageUrl)

#include "test_imageurl.moc"
