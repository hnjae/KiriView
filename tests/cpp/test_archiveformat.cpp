// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archive/archiveformat.h"

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
    QVERIFY(kiriview::isComicBookArchiveFileName(QStringLiteral("book.CBZ")));
    QVERIFY(kiriview::isComicBookArchiveFileName(QStringLiteral("book.CBT")));
    QVERIFY(kiriview::isComicBookArchiveFileName(QStringLiteral("book.CB7")));
    QVERIFY(kiriview::isComicBookArchiveFileName(QStringLiteral("book.CBR")));
    QVERIFY(!kiriview::isComicBookArchiveFileName(QStringLiteral("book.zip")));
    QVERIFY(!kiriview::isComicBookArchiveFileName(QStringLiteral("book.rar")));
}

void TestArchiveFormat::comicBookArchiveUrlsMapToKioSchemes()
{
    const QUrl cbzUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbz"));
    const QUrl cbtUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbt"));
    const QUrl cb7Url = QUrl::fromLocalFile(QStringLiteral("/books/book.cb7"));
    const QUrl cbrUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbr"));

    QCOMPARE(kiriview::comicBookArchiveKioSchemeForUrl(cbzUrl), QStringLiteral("zip"));
    QCOMPARE(kiriview::comicBookArchiveKioSchemeForUrl(cbtUrl), QStringLiteral("tar"));
    QCOMPARE(kiriview::comicBookArchiveKioSchemeForUrl(cb7Url), QStringLiteral("sevenz"));
    QCOMPARE(kiriview::comicBookArchiveKioSchemeForUrl(cbrUrl), QStringLiteral("rar"));
    QVERIFY(kiriview::isComicBookArchiveUrl(cbrUrl));
    QVERIFY(kiriview::comicBookArchiveKioSchemeForUrl(
        QUrl(QStringLiteral("smb://server/books/book.cb7")))
            .isEmpty());
}

void TestArchiveFormat::comicBookArchiveUrlMatchesExposeArchiveKind()
{
    const std::optional<kiriview::ArchiveOpenMatch> match = kiriview::comicBookArchiveMatchForUrl(
        QUrl::fromLocalFile(QStringLiteral("/books/book.cbz")));
    QVERIFY(match.has_value());
    QCOMPARE(match->scheme, QStringLiteral("zip"));
    QVERIFY(match->kind == kiriview::ArchiveOpenMatchKind::ComicBook);

    QVERIFY(!kiriview::comicBookArchiveMatchForUrl(
        QUrl::fromLocalFile(QStringLiteral("/books/book.zip")))
            .has_value());
    QVERIFY(
        !kiriview::comicBookArchiveMatchForUrl(QUrl(QStringLiteral("smb://server/books/book.cbz")))
            .has_value());
}

void TestArchiveFormat::directArchiveOpenUrlsMapGeneralArchivesToKioSchemes()
{
    const QUrl zipUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.zip"));
    const QUrl tarUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.tar"));
    const QUrl sevenZipUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.7z"));
    const QUrl rarUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.rar"));

    QCOMPARE(kiriview::directArchiveOpenKioSchemeForUrl(zipUrl), QStringLiteral("zip"));
    QCOMPARE(kiriview::directArchiveOpenKioSchemeForUrl(tarUrl), QStringLiteral("tar"));
    QCOMPARE(kiriview::directArchiveOpenKioSchemeForUrl(sevenZipUrl), QStringLiteral("sevenz"));
    QCOMPARE(kiriview::directArchiveOpenKioSchemeForUrl(rarUrl), QStringLiteral("rar"));
    QVERIFY(kiriview::directArchiveOpenKioSchemeForUrl(
        QUrl(QStringLiteral("smb://server/books/book.zip")))
            .isEmpty());
    QVERIFY(kiriview::comicBookArchiveKioSchemeForUrl(zipUrl).isEmpty());
    QVERIFY(kiriview::comicBookArchiveKioSchemeForUrl(rarUrl).isEmpty());
}

void TestArchiveFormat::directArchiveOpenMatchesExposeArchiveKind()
{
    const std::optional<kiriview::ArchiveOpenMatch> cbzFileNameMatch
        = kiriview::directArchiveOpenMatchForFileName(QStringLiteral("book.cbz"));
    QVERIFY(cbzFileNameMatch.has_value());
    QCOMPARE(cbzFileNameMatch->scheme, QStringLiteral("zip"));
    QVERIFY(cbzFileNameMatch->kind == kiriview::ArchiveOpenMatchKind::ComicBook);

    const std::optional<kiriview::ArchiveOpenMatch> zipFileNameMatch
        = kiriview::directArchiveOpenMatchForFileName(QStringLiteral("book.zip"));
    QVERIFY(zipFileNameMatch.has_value());
    QCOMPARE(zipFileNameMatch->scheme, QStringLiteral("zip"));
    QVERIFY(zipFileNameMatch->kind == kiriview::ArchiveOpenMatchKind::GeneralArchive);

    const std::optional<kiriview::ArchiveOpenMatch> cbrMimeMatch
        = kiriview::directArchiveOpenMatchForMimeTypeName(QStringLiteral("application/x-cbr"));
    QVERIFY(cbrMimeMatch.has_value());
    QCOMPARE(cbrMimeMatch->scheme, QStringLiteral("rar"));
    QVERIFY(cbrMimeMatch->kind == kiriview::ArchiveOpenMatchKind::ComicBook);

    const std::optional<kiriview::ArchiveOpenMatch> rarMimeMatch
        = kiriview::directArchiveOpenMatchForMimeTypeName(QStringLiteral("application/vnd.rar"));
    QVERIFY(rarMimeMatch.has_value());
    QCOMPARE(rarMimeMatch->scheme, QStringLiteral("rar"));
    QVERIFY(rarMimeMatch->kind == kiriview::ArchiveOpenMatchKind::GeneralArchive);

    const std::optional<kiriview::ArchiveOpenMatch> cbzUrlMatch
        = kiriview::directArchiveOpenMatchForUrl(
            QUrl::fromLocalFile(QStringLiteral("/books/book.cbz")));
    QVERIFY(cbzUrlMatch.has_value());
    QCOMPARE(cbzUrlMatch->scheme, QStringLiteral("zip"));
    QVERIFY(cbzUrlMatch->kind == kiriview::ArchiveOpenMatchKind::ComicBook);

    const std::optional<kiriview::ArchiveOpenMatch> rarUrlMatch
        = kiriview::directArchiveOpenMatchForUrl(
            QUrl::fromLocalFile(QStringLiteral("/books/book.rar")));
    QVERIFY(rarUrlMatch.has_value());
    QCOMPARE(rarUrlMatch->scheme, QStringLiteral("rar"));
    QVERIFY(rarUrlMatch->kind == kiriview::ArchiveOpenMatchKind::GeneralArchive);
}

void TestArchiveFormat::directArchiveOpenMimeTypesMapGeneralArchivesToKioSchemes()
{
    QCOMPARE(kiriview::directArchiveOpenKioSchemeForMimeTypeName(QStringLiteral("application/zip")),
        QStringLiteral("zip"));
    QCOMPARE(
        kiriview::directArchiveOpenKioSchemeForMimeTypeName(QStringLiteral("application/x-tar")),
        QStringLiteral("tar"));
    const QString sevenZipMimeType = QStringLiteral("application/x-7z-compressed");
    QCOMPARE(kiriview::directArchiveOpenKioSchemeForMimeTypeName(sevenZipMimeType),
        QStringLiteral("sevenz"));
    const QString comicBookRarMimeType = QStringLiteral("application/vnd.comicbook-rar");
    QCOMPARE(kiriview::directArchiveOpenKioSchemeForMimeTypeName(comicBookRarMimeType),
        QStringLiteral("rar"));
    const QString cbrAliasMimeType = QStringLiteral("application/x-cbr");
    QCOMPARE(kiriview::directArchiveOpenKioSchemeForMimeTypeName(cbrAliasMimeType),
        QStringLiteral("rar"));
    const QString rarMimeType = QStringLiteral("application/vnd.rar");
    QCOMPARE(
        kiriview::directArchiveOpenKioSchemeForMimeTypeName(rarMimeType), QStringLiteral("rar"));
    const QString rarAliasMimeType = QStringLiteral("application/x-rar");
    QCOMPARE(kiriview::directArchiveOpenKioSchemeForMimeTypeName(rarAliasMimeType),
        QStringLiteral("rar"));
    const QString rarCompressedAliasMimeType = QStringLiteral("application/x-rar-compressed");
    QCOMPARE(kiriview::directArchiveOpenKioSchemeForMimeTypeName(rarCompressedAliasMimeType),
        QStringLiteral("rar"));
}

void TestArchiveFormat::archiveRootSchemesReportKioFuseSupportByBackend()
{
    QVERIFY(kiriview::archiveRootSchemeUsesKioFuse(QStringLiteral("zip")));
    QVERIFY(kiriview::archiveRootSchemeUsesKioFuse(QStringLiteral("tar")));
    QVERIFY(kiriview::archiveRootSchemeUsesKioFuse(QStringLiteral("sevenz")));
    QVERIFY(!kiriview::archiveRootSchemeUsesKioFuse(QStringLiteral("rar")));
    QVERIFY(!kiriview::archiveRootSchemeUsesKioFuse(QStringLiteral("unknown")));
}

QTEST_GUILESS_MAIN(TestArchiveFormat)

#include "test_archiveformat.moc"
