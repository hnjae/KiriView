// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageformatregistry.h"

#include "archiveformat.h"

#include <QFile>
#include <QObject>
#include <QStringList>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
QStringList sortedUnique(QStringList values)
{
    values.removeAll(QString());
    values.sort();
    values.removeDuplicates();
    return values;
}
}

class TestImageFormatRegistry : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void supportedImageExtensionsIncludeAdvertisedFormats();
    void supportedOpenExtensionsIncludeComicBookArchives();
    void supportedOpenExtensionsDoNotAdvertiseGeneralArchives();
    void desktopMimeTypesMatchSupportedOpenMimeTypes();
    void comicBookArchiveFileNamesAreCaseInsensitive();
    void comicBookArchiveUrlsMapToKioSchemes();
    void directArchiveOpenUrlsMapGeneralArchivesToKioSchemes();
    void directArchiveOpenMatchesExposeArchiveKind();
    void directArchiveOpenMimeTypesMapGeneralArchivesToKioSchemes();
    void archiveRootSchemesReportKioFuseSupportByBackend();
};

void TestImageFormatRegistry::supportedImageExtensionsIncludeAdvertisedFormats()
{
    const QStringList extensions = KiriView::supportedImageExtensions();

    QVERIFY(extensions.contains(QStringLiteral("avci")));
    QVERIFY(extensions.contains(QStringLiteral("hej2")));
    QVERIFY(extensions.contains(QStringLiteral("heics")));
    QVERIFY(extensions.contains(QStringLiteral("heifs")));
    QVERIFY(extensions.contains(QStringLiteral("tif")));
    QVERIFY(extensions.contains(QStringLiteral("tiff")));
    QVERIFY(KiriView::isSupportedImageFileName(QStringLiteral("still.AVCI")));
    QVERIFY(KiriView::isSupportedImageFileName(QStringLiteral("still.HEJ2")));
    QVERIFY(KiriView::isSupportedImageFileName(QStringLiteral("sequence.HEICS")));
    QVERIFY(KiriView::isSupportedImageFileName(QStringLiteral("sequence.HEIFS")));
    QVERIFY(KiriView::isSupportedImageFileName(QStringLiteral("scan.TIF")));
    QVERIFY(KiriView::isSupportedImageFileName(QStringLiteral("scan.TIFF")));
}

void TestImageFormatRegistry::supportedOpenExtensionsIncludeComicBookArchives()
{
    const QStringList extensions = KiriView::supportedOpenExtensions();

    QVERIFY(extensions.contains(QStringLiteral("cbz")));
    QVERIFY(extensions.contains(QStringLiteral("cbt")));
    QVERIFY(extensions.contains(QStringLiteral("cb7")));
    QVERIFY(extensions.contains(QStringLiteral("cbr")));
}

void TestImageFormatRegistry::supportedOpenExtensionsDoNotAdvertiseGeneralArchives()
{
    const QStringList extensions = KiriView::supportedOpenExtensions();

    QVERIFY(!extensions.contains(QStringLiteral("zip")));
    QVERIFY(!extensions.contains(QStringLiteral("tar")));
    QVERIFY(!extensions.contains(QStringLiteral("7z")));
    QVERIFY(!extensions.contains(QStringLiteral("rar")));
}

void TestImageFormatRegistry::desktopMimeTypesMatchSupportedOpenMimeTypes()
{
    QFile desktopFile(
        QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR "/../../io.github.hnjae.KiriView.desktop"));
    QVERIFY(desktopFile.open(QIODevice::ReadOnly | QIODevice::Text));

    const QString mimePrefix = QStringLiteral("MimeType=");
    QString mimeLine;
    const QString desktopText = QString::fromUtf8(desktopFile.readAll());
    for (const QString &line : desktopText.split(QLatin1Char('\n'))) {
        if (line.startsWith(mimePrefix)) {
            mimeLine = line.mid(mimePrefix.size());
            break;
        }
    }
    QVERIFY(!mimeLine.isEmpty());

    QStringList expectedMimeTypes = KiriView::supportedImageMimeTypes();
    expectedMimeTypes.append(KiriView::supportedComicBookArchiveMimeTypes());

    QCOMPARE(sortedUnique(mimeLine.split(QLatin1Char(';'), Qt::SkipEmptyParts)),
        sortedUnique(expectedMimeTypes));
}

void TestImageFormatRegistry::comicBookArchiveFileNamesAreCaseInsensitive()
{
    QVERIFY(KiriView::isComicBookArchiveFileName(QStringLiteral("book.CBZ")));
    QVERIFY(KiriView::isComicBookArchiveFileName(QStringLiteral("book.CBT")));
    QVERIFY(KiriView::isComicBookArchiveFileName(QStringLiteral("book.CB7")));
    QVERIFY(KiriView::isComicBookArchiveFileName(QStringLiteral("book.CBR")));
    QVERIFY(!KiriView::isComicBookArchiveFileName(QStringLiteral("book.zip")));
    QVERIFY(!KiriView::isComicBookArchiveFileName(QStringLiteral("book.rar")));
}

void TestImageFormatRegistry::comicBookArchiveUrlsMapToKioSchemes()
{
    const QUrl cbzUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbz"));
    const QUrl cbtUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbt"));
    const QUrl cb7Url = QUrl::fromLocalFile(QStringLiteral("/books/book.cb7"));
    const QUrl cbrUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbr"));

    QCOMPARE(KiriView::comicBookArchiveKioSchemeForUrl(cbzUrl), QStringLiteral("zip"));
    QCOMPARE(KiriView::comicBookArchiveKioSchemeForUrl(cbtUrl), QStringLiteral("tar"));
    QCOMPARE(KiriView::comicBookArchiveKioSchemeForUrl(cb7Url), QStringLiteral("sevenz"));
    QCOMPARE(KiriView::comicBookArchiveKioSchemeForUrl(cbrUrl), QStringLiteral("rar"));
    QVERIFY(KiriView::comicBookArchiveKioSchemeForUrl(
        QUrl(QStringLiteral("smb://server/books/book.cb7")))
            .isEmpty());
}

void TestImageFormatRegistry::directArchiveOpenUrlsMapGeneralArchivesToKioSchemes()
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

void TestImageFormatRegistry::directArchiveOpenMatchesExposeArchiveKind()
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
}

void TestImageFormatRegistry::directArchiveOpenMimeTypesMapGeneralArchivesToKioSchemes()
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

void TestImageFormatRegistry::archiveRootSchemesReportKioFuseSupportByBackend()
{
    QVERIFY(KiriView::archiveRootSchemeUsesKioFuse(QStringLiteral("zip")));
    QVERIFY(KiriView::archiveRootSchemeUsesKioFuse(QStringLiteral("tar")));
    QVERIFY(KiriView::archiveRootSchemeUsesKioFuse(QStringLiteral("sevenz")));
    QVERIFY(!KiriView::archiveRootSchemeUsesKioFuse(QStringLiteral("rar")));
    QVERIFY(!KiriView::archiveRootSchemeUsesKioFuse(QStringLiteral("unknown")));
}

QTEST_GUILESS_MAIN(TestImageFormatRegistry)

#include "test_imageformatregistry.moc"
