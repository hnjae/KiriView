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
kiriview::OpenedCollectionScopeLocation openedCollectionScope()
{
    return kiriview::OpenedCollectionScopeLocation::fromUrls(
        QUrl::fromLocalFile(QStringLiteral("/books/book.cbz")),
        QUrl(QStringLiteral("zip:///books/book.cbz/")),
        kiriview::OpenedCollectionScopeKind::ComicBookArchive);
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
    kiriview::OpenedCollectionScopeLocation archive = openedCollectionScope();
    QUrl rootUrl = archive.rootUrl();
    rootUrl.setQuery(QStringLiteral("token=ignored"));
    rootUrl.setFragment(QStringLiteral("ignored"));
    archive = kiriview::OpenedCollectionScopeLocation::fromUrls(
        archive.fileUrl(), rootUrl, archive.kind());

    const QUrl url
        = kiriview::openedCollectionEntryUrl(archive, QStringLiteral("./chapter/./page001.png"));

    QCOMPARE(url, QUrl(QStringLiteral("zip:///books/book.cbz/chapter/page001.png")));
    QVERIFY(url.query().isEmpty());
    QVERIFY(url.fragment().isEmpty());
}

void TestArchivePath::entryUrlsRejectUnsafePaths()
{
    const kiriview::OpenedCollectionScopeLocation archive = openedCollectionScope();

    QVERIFY(
        kiriview::openedCollectionEntryUrl(archive, QStringLiteral("../page001.png")).isEmpty());
    QVERIFY(
        kiriview::openedCollectionEntryUrl(archive, QStringLiteral("/tmp/page001.png")).isEmpty());
    QVERIFY(kiriview::openedCollectionEntryUrl(
        kiriview::OpenedCollectionScopeLocation::none(), QStringLiteral("page001.png"))
            .isEmpty());
}

void TestArchivePath::entryPathsResolveOnlyInsideArchiveRoot()
{
    const kiriview::OpenedCollectionScopeLocation archive = openedCollectionScope();

    QCOMPARE(kiriview::openedCollectionEntryPathForUrl(
                 archive, QUrl(QStringLiteral("zip:///books/book.cbz/chapter/page001.png"))),
        QStringLiteral("chapter/page001.png"));
    QVERIFY(kiriview::openedCollectionEntryPathForUrl(
        archive, QUrl(QStringLiteral("zip:///books/book.cbz/")))
            .isEmpty());
    QVERIFY(kiriview::openedCollectionEntryPathForUrl(
        archive, QUrl(QStringLiteral("zip:///books/book.cbz/../page001.png")))
            .isEmpty());
    QVERIFY(kiriview::openedCollectionEntryPathForUrl(
        archive, QUrl(QStringLiteral("tar:///books/book.cbz/chapter/page001.png")))
            .isEmpty());
}

void TestArchivePath::kioFuseArchivePathsExposeParsedArchiveParts()
{
    const std::optional<kiriview::KioFuseArchivePath> parsed = kiriview::kioFuseArchivePath(
        QStringLiteral("/run/user/1000/kio-fuse-test/zip/books/book.cbz/page001.png"),
        QStringLiteral("/run/user/1000"));

    QVERIFY(parsed.has_value());
    QCOMPARE(parsed->scheme, QStringLiteral("zip"));
    QCOMPARE(parsed->path, QStringLiteral("/books/book.cbz/page001.png"));

    QVERIFY(!kiriview::kioFuseArchivePath(
        QStringLiteral("/tmp/kio-fuse-test/zip/books/book.cbz/page001.png"),
        QStringLiteral("/run/user/1000"))
            .has_value());
}

void TestArchivePath::kioFuseArchiveUrlsRejectUnsupportedOrMalformedPaths()
{
    const QString runtimeDir = QStringLiteral("/run/user/1000");

    const std::optional<QUrl> url = kiriview::kioFuseArchiveUrlForLocalPath(
        QStringLiteral("/run/user/1000/kio-fuse-test/sevenz/books/book.cb7/page001.png"),
        runtimeDir);
    QVERIFY(url.has_value());
    QCOMPARE(url->scheme(), QStringLiteral("sevenz"));
    QCOMPARE(url->path(), QStringLiteral("/books/book.cb7/page001.png"));

    QVERIFY(!kiriview::kioFuseArchiveUrlForLocalPath(
        QStringLiteral("/run/user/1000/kio-fuse-test/rar/books/book.cbr/page001.png"), runtimeDir)
            .has_value());
    QVERIFY(!kiriview::kioFuseArchiveUrlForLocalPath(
        QStringLiteral("/run/user/1000/kio-fuse-test/zip"), runtimeDir)
            .has_value());
}

QTEST_GUILESS_MAIN(TestArchivePath)

#include "test_archivepath.moc"
