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
    void comicBookArchiveFileNamesAreCaseInsensitive();
    void comicBookArchiveUrlsMapToKioSchemes();
};

void TestImageFormatRegistry::supportedOpenExtensionsIncludeComicBookArchives()
{
    const QStringList extensions = KiriView::supportedOpenExtensions();

    QVERIFY(extensions.contains(QStringLiteral("cbz")));
    QVERIFY(extensions.contains(QStringLiteral("cbt")));
    QVERIFY(extensions.contains(QStringLiteral("cb7")));
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

QTEST_GUILESS_MAIN(TestImageFormatRegistry)

#include "test_imageformatregistry.moc"
