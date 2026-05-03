// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageformatregistry.h"

#include <QObject>
#include <QStringList>
#include <QTest>
#include <QUrl>

class TestImageFormatRegistry : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void supportedOpenExtensionsIncludeComicBookArchives();
    void supportedOpenExtensionsDoNotAdvertiseGeneralArchives();
    void comicBookArchiveFileNamesAreCaseInsensitive();
    void comicBookArchiveUrlsMapToKioSchemes();
    void directArchiveOpenUrlsMapGeneralArchivesToKioSchemes();
    void directArchiveOpenMimeTypesMapGeneralArchivesToKioSchemes();
};

void TestImageFormatRegistry::supportedOpenExtensionsIncludeComicBookArchives()
{
    const QStringList extensions = KiriView::supportedOpenExtensions();

    QVERIFY(extensions.contains(QStringLiteral("cbz")));
    QVERIFY(extensions.contains(QStringLiteral("cbt")));
    QVERIFY(extensions.contains(QStringLiteral("cb7")));
}

void TestImageFormatRegistry::supportedOpenExtensionsDoNotAdvertiseGeneralArchives()
{
    const QStringList extensions = KiriView::supportedOpenExtensions();

    QVERIFY(!extensions.contains(QStringLiteral("zip")));
    QVERIFY(!extensions.contains(QStringLiteral("tar")));
    QVERIFY(!extensions.contains(QStringLiteral("7z")));
}

void TestImageFormatRegistry::comicBookArchiveFileNamesAreCaseInsensitive()
{
    QVERIFY(KiriView::isComicBookArchiveFileName(QStringLiteral("book.CBZ")));
    QVERIFY(KiriView::isComicBookArchiveFileName(QStringLiteral("book.CBT")));
    QVERIFY(KiriView::isComicBookArchiveFileName(QStringLiteral("book.CB7")));
    QVERIFY(!KiriView::isComicBookArchiveFileName(QStringLiteral("book.zip")));
}

void TestImageFormatRegistry::comicBookArchiveUrlsMapToKioSchemes()
{
    const QUrl cbzUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbz"));
    const QUrl cbtUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbt"));
    const QUrl cb7Url = QUrl::fromLocalFile(QStringLiteral("/books/book.cb7"));

    QCOMPARE(KiriView::comicBookArchiveKioSchemeForUrl(cbzUrl), QStringLiteral("zip"));
    QCOMPARE(KiriView::comicBookArchiveKioSchemeForUrl(cbtUrl), QStringLiteral("tar"));
    QCOMPARE(KiriView::comicBookArchiveKioSchemeForUrl(cb7Url), QStringLiteral("sevenz"));
    QVERIFY(KiriView::comicBookArchiveKioSchemeForUrl(
        QUrl(QStringLiteral("smb://server/books/book.cb7")))
            .isEmpty());
}

void TestImageFormatRegistry::directArchiveOpenUrlsMapGeneralArchivesToKioSchemes()
{
    const QUrl zipUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.zip"));
    const QUrl tarUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.tar"));
    const QUrl sevenZipUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.7z"));

    QCOMPARE(KiriView::directArchiveOpenKioSchemeForUrl(zipUrl), QStringLiteral("zip"));
    QCOMPARE(KiriView::directArchiveOpenKioSchemeForUrl(tarUrl), QStringLiteral("tar"));
    QCOMPARE(KiriView::directArchiveOpenKioSchemeForUrl(sevenZipUrl), QStringLiteral("sevenz"));
    QVERIFY(KiriView::directArchiveOpenKioSchemeForUrl(
        QUrl(QStringLiteral("smb://server/books/book.zip")))
            .isEmpty());
    QVERIFY(KiriView::comicBookArchiveKioSchemeForUrl(zipUrl).isEmpty());
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
}

QTEST_GUILESS_MAIN(TestImageFormatRegistry)

#include "test_imageformatregistry.moc"
