// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloadtypes.h"
#include "imagelocation.h"
#include "imageurl.h"

#include <QByteArray>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <QtGlobal>
#include <optional>

namespace {
QUrl archiveUrl(const QString &scheme, const QString &path)
{
    QUrl url;
    url.setScheme(scheme);
    url.setPath(path);
    return url;
}
}

class TestImageUrl : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void normalizedContainerUrlsStripQueryFragmentsAndCleanLocalPaths();
    void normalizedUrlIdentityHelpersRejectInvalidImageUrlsAndPreserveKeyFormatting();
    void sameNormalizedUrlMatchesPathSegments();
    void sameContainerNavigationUrlMatchesNormalizedPaths();
    void parentUrlForContainerNavigationHandlesContainers();
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

    const KiriView::ArchiveDocumentLocation archiveDocument
        = KiriView::ArchiveDocumentLocation::fromUrls(
            QUrl::fromLocalFile(QStringLiteral("/books/book.cbz")),
            QUrl(QStringLiteral("zip:///books/book.cbz/")),
            KiriView::ArchiveDocumentKind::ComicBook);
    const KiriView::DisplayedImageLocation location
        = KiriView::DisplayedImageLocation::fromArchiveDocument(
            QUrl(QStringLiteral("zip:///books/book.cbz/page.png")), archiveDocument);
    QVERIFY(!location.isEmpty());
    QCOMPARE(location.archiveDocumentFileUrl(), archiveDocument.fileUrl());
    QCOMPARE(location.archiveDocumentRootUrl(), archiveDocument.rootUrl());

    const KiriView::ImageLoadRequest plainOpen
        = KiriView::ImageLoadRequest::fromUrl(location.imageUrl());
    QVERIFY(!plainOpen.isEmpty());
    QVERIFY(!plainOpen.isContainerNavigation());
    QCOMPARE(plainOpen.sourceUrl(), location.imageUrl());
    QVERIFY(plainOpen.archiveDocument().rootUrl().isEmpty());

    const QUrl containerUrl = QUrl::fromLocalFile(QStringLiteral("/images/"));
    const KiriView::ImageLoadRequest containerOpen = KiriView::ImageLoadRequest::fromLocation(
        location.imageUrl(), location.archiveDocument(), containerUrl);
    QVERIFY(containerOpen.isContainerNavigation());
    QCOMPARE(containerOpen.archiveDocument().rootUrl(), location.archiveDocumentRootUrl());
    QCOMPARE(containerOpen.containerNavigationUrl(), containerUrl);
}

QTEST_GUILESS_MAIN(TestImageUrl)

#include "test_imageurl.moc"
