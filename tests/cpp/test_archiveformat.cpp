// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archiveformat.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>

class TestArchiveFormat : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void comicBookArchiveFileNamesAreCaseInsensitive();
    void comicBookArchiveUrlsMapToKioSchemes();
    void comicBookArchiveUrlMatchesExposeArchiveKind();
    void directArchiveOpenUrlsMapGeneralArchivesToKioSchemes();
    void directArchiveOpenMatchesExposeArchiveKind();
    void directArchiveOpenMimeTypesMapGeneralArchivesToKioSchemes();
    void archiveRootSchemesReportKioFuseSupportByBackend();
};

void TestArchiveFormat::comicBookArchiveFileNamesAreCaseInsensitive()
{
    QVERIFY(KiriView::isComicBookArchiveFileName(QStringLiteral("book.CBZ")));
    QVERIFY(KiriView::isComicBookArchiveFileName(QStringLiteral("book.CBT")));
    QVERIFY(KiriView::isComicBookArchiveFileName(QStringLiteral("book.CB7")));
    QVERIFY(KiriView::isComicBookArchiveFileName(QStringLiteral("book.CBR")));
    QVERIFY(!KiriView::isComicBookArchiveFileName(QStringLiteral("book.zip")));
    QVERIFY(!KiriView::isComicBookArchiveFileName(QStringLiteral("book.rar")));
}

void TestArchiveFormat::comicBookArchiveUrlsMapToKioSchemes()
{
    const QUrl cbzUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbz"));
    const QUrl cbtUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbt"));
    const QUrl cb7Url = QUrl::fromLocalFile(QStringLiteral("/books/book.cb7"));
    const QUrl cbrUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbr"));

    QCOMPARE(KiriView::comicBookArchiveKioSchemeForUrl(cbzUrl), QStringLiteral("zip"));
    QCOMPARE(KiriView::comicBookArchiveKioSchemeForUrl(cbtUrl), QStringLiteral("tar"));
    QCOMPARE(KiriView::comicBookArchiveKioSchemeForUrl(cb7Url), QStringLiteral("sevenz"));
    QCOMPARE(KiriView::comicBookArchiveKioSchemeForUrl(cbrUrl), QStringLiteral("rar"));
    QVERIFY(KiriView::isComicBookArchiveUrl(cbrUrl));
    QVERIFY(KiriView::comicBookArchiveKioSchemeForUrl(
        QUrl(QStringLiteral("smb://server/books/book.cb7")))
            .isEmpty());
}

void TestArchiveFormat::comicBookArchiveUrlMatchesExposeArchiveKind()
{
    const std::optional<KiriView::ArchiveOpenMatch> match = KiriView::comicBookArchiveMatchForUrl(
        QUrl::fromLocalFile(QStringLiteral("/books/book.cbz")));
    QVERIFY(match.has_value());
    QCOMPARE(match->scheme, QStringLiteral("zip"));
    QVERIFY(match->kind == KiriView::ArchiveOpenMatchKind::ComicBook);

    QVERIFY(!KiriView::comicBookArchiveMatchForUrl(
        QUrl::fromLocalFile(QStringLiteral("/books/book.zip")))
            .has_value());
    QVERIFY(
        !KiriView::comicBookArchiveMatchForUrl(QUrl(QStringLiteral("smb://server/books/book.cbz")))
            .has_value());
}

void TestArchiveFormat::directArchiveOpenUrlsMapGeneralArchivesToKioSchemes()
{
    const QUrl zipUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.zip"));
    const QUrl tarUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.tar"));
    const QUrl sevenZipUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.7z"));
    const QUrl rarUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.rar"));

    QCOMPARE(KiriView::directArchiveOpenKioSchemeForUrl(zipUrl), QStringLiteral("zip"));
    QCOMPARE(KiriView::directArchiveOpenKioSchemeForUrl(tarUrl), QStringLiteral("tar"));
    QCOMPARE(KiriView::directArchiveOpenKioSchemeForUrl(sevenZipUrl), QStringLiteral("sevenz"));
    QCOMPARE(KiriView::directArchiveOpenKioSchemeForUrl(rarUrl), QStringLiteral("rar"));
    QVERIFY(KiriView::directArchiveOpenKioSchemeForUrl(
        QUrl(QStringLiteral("smb://server/books/book.zip")))
            .isEmpty());
    QVERIFY(KiriView::comicBookArchiveKioSchemeForUrl(zipUrl).isEmpty());
    QVERIFY(KiriView::comicBookArchiveKioSchemeForUrl(rarUrl).isEmpty());
}

void TestArchiveFormat::directArchiveOpenMatchesExposeArchiveKind()
{
    const std::optional<KiriView::ArchiveOpenMatch> cbzFileNameMatch
        = KiriView::directArchiveOpenMatchForFileName(QStringLiteral("book.cbz"));
    QVERIFY(cbzFileNameMatch.has_value());
    QCOMPARE(cbzFileNameMatch->scheme, QStringLiteral("zip"));
    QVERIFY(cbzFileNameMatch->kind == KiriView::ArchiveOpenMatchKind::ComicBook);

    const std::optional<KiriView::ArchiveOpenMatch> zipFileNameMatch
        = KiriView::directArchiveOpenMatchForFileName(QStringLiteral("book.zip"));
    QVERIFY(zipFileNameMatch.has_value());
    QCOMPARE(zipFileNameMatch->scheme, QStringLiteral("zip"));
    QVERIFY(zipFileNameMatch->kind == KiriView::ArchiveOpenMatchKind::GeneralArchive);

    const std::optional<KiriView::ArchiveOpenMatch> cbrMimeMatch
        = KiriView::directArchiveOpenMatchForMimeTypeName(QStringLiteral("application/x-cbr"));
    QVERIFY(cbrMimeMatch.has_value());
    QCOMPARE(cbrMimeMatch->scheme, QStringLiteral("rar"));
    QVERIFY(cbrMimeMatch->kind == KiriView::ArchiveOpenMatchKind::ComicBook);

    const std::optional<KiriView::ArchiveOpenMatch> rarMimeMatch
        = KiriView::directArchiveOpenMatchForMimeTypeName(QStringLiteral("application/vnd.rar"));
    QVERIFY(rarMimeMatch.has_value());
    QCOMPARE(rarMimeMatch->scheme, QStringLiteral("rar"));
    QVERIFY(rarMimeMatch->kind == KiriView::ArchiveOpenMatchKind::GeneralArchive);

    const std::optional<KiriView::ArchiveOpenMatch> cbzUrlMatch
        = KiriView::directArchiveOpenMatchForUrl(
            QUrl::fromLocalFile(QStringLiteral("/books/book.cbz")));
    QVERIFY(cbzUrlMatch.has_value());
    QCOMPARE(cbzUrlMatch->scheme, QStringLiteral("zip"));
    QVERIFY(cbzUrlMatch->kind == KiriView::ArchiveOpenMatchKind::ComicBook);

    const std::optional<KiriView::ArchiveOpenMatch> rarUrlMatch
        = KiriView::directArchiveOpenMatchForUrl(
            QUrl::fromLocalFile(QStringLiteral("/books/book.rar")));
    QVERIFY(rarUrlMatch.has_value());
    QCOMPARE(rarUrlMatch->scheme, QStringLiteral("rar"));
    QVERIFY(rarUrlMatch->kind == KiriView::ArchiveOpenMatchKind::GeneralArchive);
}

void TestArchiveFormat::directArchiveOpenMimeTypesMapGeneralArchivesToKioSchemes()
{
    QCOMPARE(KiriView::directArchiveOpenKioSchemeForMimeTypeName(QStringLiteral("application/zip")),
        QStringLiteral("zip"));
    QCOMPARE(
        KiriView::directArchiveOpenKioSchemeForMimeTypeName(QStringLiteral("application/x-tar")),
        QStringLiteral("tar"));
    const QString sevenZipMimeType = QStringLiteral("application/x-7z-compressed");
    QCOMPARE(KiriView::directArchiveOpenKioSchemeForMimeTypeName(sevenZipMimeType),
        QStringLiteral("sevenz"));
    const QString comicBookRarMimeType = QStringLiteral("application/vnd.comicbook-rar");
    QCOMPARE(KiriView::directArchiveOpenKioSchemeForMimeTypeName(comicBookRarMimeType),
        QStringLiteral("rar"));
    const QString cbrAliasMimeType = QStringLiteral("application/x-cbr");
    QCOMPARE(KiriView::directArchiveOpenKioSchemeForMimeTypeName(cbrAliasMimeType),
        QStringLiteral("rar"));
    const QString rarMimeType = QStringLiteral("application/vnd.rar");
    QCOMPARE(
        KiriView::directArchiveOpenKioSchemeForMimeTypeName(rarMimeType), QStringLiteral("rar"));
    const QString rarAliasMimeType = QStringLiteral("application/x-rar");
    QCOMPARE(KiriView::directArchiveOpenKioSchemeForMimeTypeName(rarAliasMimeType),
        QStringLiteral("rar"));
    const QString rarCompressedAliasMimeType = QStringLiteral("application/x-rar-compressed");
    QCOMPARE(KiriView::directArchiveOpenKioSchemeForMimeTypeName(rarCompressedAliasMimeType),
        QStringLiteral("rar"));
}

void TestArchiveFormat::archiveRootSchemesReportKioFuseSupportByBackend()
{
    QVERIFY(KiriView::archiveRootSchemeUsesKioFuse(QStringLiteral("zip")));
    QVERIFY(KiriView::archiveRootSchemeUsesKioFuse(QStringLiteral("tar")));
    QVERIFY(KiriView::archiveRootSchemeUsesKioFuse(QStringLiteral("sevenz")));
    QVERIFY(!KiriView::archiveRootSchemeUsesKioFuse(QStringLiteral("rar")));
    QVERIFY(!KiriView::archiveRootSchemeUsesKioFuse(QStringLiteral("unknown")));
}

QTEST_GUILESS_MAIN(TestArchiveFormat)

#include "test_archiveformat.moc"
