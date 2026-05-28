// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archive/archivepath.h"
#include "location/imagelocation.h"

#include <QObject>
#include <QString>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
KiriView::OpenedCollectionScopeLocation openedCollectionScope()
{
    return KiriView::OpenedCollectionScopeLocation::fromUrls(
        QUrl::fromLocalFile(QStringLiteral("/books/book.cbz")),
        QUrl(QStringLiteral("zip:///books/book.cbz/")),
        KiriView::OpenedCollectionScopeKind::ComicBookArchive);
}
}

class TestArchivePath : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void entryUrlsNormalizePathsAndClearUrlMetadata();
    void entryUrlsRejectUnsafePaths();
    void entryPathsResolveOnlyInsideArchiveRoot();
    void kioFuseArchivePathsExposeParsedArchiveParts();
    void kioFuseArchiveUrlsRejectUnsupportedOrMalformedPaths();
};

void TestArchivePath::entryUrlsNormalizePathsAndClearUrlMetadata()
{
    KiriView::OpenedCollectionScopeLocation archive = openedCollectionScope();
    QUrl rootUrl = archive.rootUrl();
    rootUrl.setQuery(QStringLiteral("token=ignored"));
    rootUrl.setFragment(QStringLiteral("ignored"));
    archive = KiriView::OpenedCollectionScopeLocation::fromUrls(
        archive.fileUrl(), rootUrl, archive.kind());

    const QUrl url
        = KiriView::openedCollectionEntryUrl(archive, QStringLiteral("./chapter/./page001.png"));

    QCOMPARE(url, QUrl(QStringLiteral("zip:///books/book.cbz/chapter/page001.png")));
    QVERIFY(url.query().isEmpty());
    QVERIFY(url.fragment().isEmpty());
}

void TestArchivePath::entryUrlsRejectUnsafePaths()
{
    const KiriView::OpenedCollectionScopeLocation archive = openedCollectionScope();

    QVERIFY(
        KiriView::openedCollectionEntryUrl(archive, QStringLiteral("../page001.png")).isEmpty());
    QVERIFY(
        KiriView::openedCollectionEntryUrl(archive, QStringLiteral("/tmp/page001.png")).isEmpty());
    QVERIFY(KiriView::openedCollectionEntryUrl(
        KiriView::OpenedCollectionScopeLocation::none(), QStringLiteral("page001.png"))
            .isEmpty());
}

void TestArchivePath::entryPathsResolveOnlyInsideArchiveRoot()
{
    const KiriView::OpenedCollectionScopeLocation archive = openedCollectionScope();

    QCOMPARE(KiriView::openedCollectionEntryPathForUrl(
                 archive, QUrl(QStringLiteral("zip:///books/book.cbz/chapter/page001.png"))),
        QStringLiteral("chapter/page001.png"));
    QVERIFY(KiriView::openedCollectionEntryPathForUrl(
        archive, QUrl(QStringLiteral("zip:///books/book.cbz/")))
            .isEmpty());
    QVERIFY(KiriView::openedCollectionEntryPathForUrl(
        archive, QUrl(QStringLiteral("zip:///books/book.cbz/../page001.png")))
            .isEmpty());
    QVERIFY(KiriView::openedCollectionEntryPathForUrl(
        archive, QUrl(QStringLiteral("tar:///books/book.cbz/chapter/page001.png")))
            .isEmpty());
}

void TestArchivePath::kioFuseArchivePathsExposeParsedArchiveParts()
{
    const std::optional<KiriView::KioFuseArchivePath> parsed = KiriView::kioFuseArchivePath(
        QStringLiteral("/run/user/1000/kio-fuse-test/zip/books/book.cbz/page001.png"),
        QStringLiteral("/run/user/1000"));

    QVERIFY(parsed.has_value());
    QCOMPARE(parsed->scheme, QStringLiteral("zip"));
    QCOMPARE(parsed->path, QStringLiteral("/books/book.cbz/page001.png"));

    QVERIFY(!KiriView::kioFuseArchivePath(
        QStringLiteral("/tmp/kio-fuse-test/zip/books/book.cbz/page001.png"),
        QStringLiteral("/run/user/1000"))
            .has_value());
}

void TestArchivePath::kioFuseArchiveUrlsRejectUnsupportedOrMalformedPaths()
{
    const QString runtimeDir = QStringLiteral("/run/user/1000");

    const std::optional<QUrl> url = KiriView::kioFuseArchiveUrlForLocalPath(
        QStringLiteral("/run/user/1000/kio-fuse-test/sevenz/books/book.cb7/page001.png"),
        runtimeDir);
    QVERIFY(url.has_value());
    QCOMPARE(url->scheme(), QStringLiteral("sevenz"));
    QCOMPARE(url->path(), QStringLiteral("/books/book.cb7/page001.png"));

    QVERIFY(!KiriView::kioFuseArchiveUrlForLocalPath(
        QStringLiteral("/run/user/1000/kio-fuse-test/rar/books/book.cbr/page001.png"), runtimeDir)
            .has_value());
    QVERIFY(!KiriView::kioFuseArchiveUrlForLocalPath(
        QStringLiteral("/run/user/1000/kio-fuse-test/zip"), runtimeDir)
            .has_value());
}

QTEST_GUILESS_MAIN(TestArchivePath)

#include "test_archivepath.moc"
