// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivepath.h"
#include "imagelocation.h"

#include <QObject>
#include <QString>
#include <QTest>
#include <QUrl>

namespace {
KiriView::ArchiveDocumentLocation archiveDocument()
{
    return KiriView::ArchiveDocumentLocation::fromUrls(
        QUrl::fromLocalFile(QStringLiteral("/books/book.cbz")),
        QUrl(QStringLiteral("zip:///books/book.cbz/")), KiriView::ArchiveDocumentKind::ComicBook);
}
}

class TestArchivePath : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void entryUrlsNormalizePathsAndClearUrlMetadata();
    void entryUrlsRejectUnsafePaths();
    void entryPathsResolveOnlyInsideArchiveRoot();
};

void TestArchivePath::entryUrlsNormalizePathsAndClearUrlMetadata()
{
    KiriView::ArchiveDocumentLocation archive = archiveDocument();
    QUrl rootUrl = archive.rootUrl();
    rootUrl.setQuery(QStringLiteral("token=ignored"));
    rootUrl.setFragment(QStringLiteral("ignored"));
    archive
        = KiriView::ArchiveDocumentLocation::fromUrls(archive.fileUrl(), rootUrl, archive.kind());

    const QUrl url = KiriView::archiveEntryUrl(archive, QStringLiteral("./chapter/./page001.png"));

    QCOMPARE(url, QUrl(QStringLiteral("zip:///books/book.cbz/chapter/page001.png")));
    QVERIFY(url.query().isEmpty());
    QVERIFY(url.fragment().isEmpty());
}

void TestArchivePath::entryUrlsRejectUnsafePaths()
{
    const KiriView::ArchiveDocumentLocation archive = archiveDocument();

    QVERIFY(KiriView::archiveEntryUrl(archive, QStringLiteral("../page001.png")).isEmpty());
    QVERIFY(KiriView::archiveEntryUrl(archive, QStringLiteral("/tmp/page001.png")).isEmpty());
    QVERIFY(KiriView::archiveEntryUrl(
        KiriView::ArchiveDocumentLocation::none(), QStringLiteral("page001.png"))
            .isEmpty());
}

void TestArchivePath::entryPathsResolveOnlyInsideArchiveRoot()
{
    const KiriView::ArchiveDocumentLocation archive = archiveDocument();

    QCOMPARE(KiriView::archiveEntryPathForUrl(
                 archive, QUrl(QStringLiteral("zip:///books/book.cbz/chapter/page001.png"))),
        QStringLiteral("chapter/page001.png"));
    QVERIFY(
        KiriView::archiveEntryPathForUrl(archive, QUrl(QStringLiteral("zip:///books/book.cbz/")))
            .isEmpty());
    QVERIFY(KiriView::archiveEntryPathForUrl(
        archive, QUrl(QStringLiteral("zip:///books/book.cbz/../page001.png")))
            .isEmpty());
    QVERIFY(KiriView::archiveEntryPathForUrl(
        archive, QUrl(QStringLiteral("tar:///books/book.cbz/chapter/page001.png")))
            .isEmpty());
}

QTEST_GUILESS_MAIN(TestArchivePath)

#include "test_archivepath.moc"
