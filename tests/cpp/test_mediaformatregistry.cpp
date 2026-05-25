// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/mediaformatregistry.h"

#include "archive/archiveformat.h"

#include <QFile>
#include <QObject>
#include <QStringList>
#include <QTest>

namespace {
QStringList sortedUnique(QStringList values)
{
    values.removeAll(QString());
    values.sort();
    values.removeDuplicates();
    return values;
}
}

class TestMediaFormatRegistry : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ordinaryMediaExtensionsIncludeImagesAndDirectVideos();
    void ordinaryMediaMimeTypesIncludeImagesAndDirectVideos();
    void ordinaryMediaFileNamesIncludeImagesAndDirectVideos();
    void directVideoFileNamesStayExposedThroughMediaRegistry();
    void openDialogFilterIncludesMediaAndArchives();
    void desktopMimeTypesMatchSupportedOpenMimeTypes();
};

void TestMediaFormatRegistry::ordinaryMediaExtensionsIncludeImagesAndDirectVideos()
{
    const QStringList extensions = KiriView::supportedOrdinaryMediaExtensions();

    QVERIFY(extensions.contains(QStringLiteral("png")));
    QVERIFY(extensions.contains(QStringLiteral("mp4")));
    QVERIFY(extensions.contains(QStringLiteral("m4v")));
    QVERIFY(extensions.contains(QStringLiteral("mov")));

    QStringList unique = extensions;
    unique.removeDuplicates();
    QCOMPARE(extensions, unique);

    QStringList sorted = extensions;
    sorted.sort();
    QCOMPARE(extensions, sorted);
}

void TestMediaFormatRegistry::ordinaryMediaMimeTypesIncludeImagesAndDirectVideos()
{
    const QStringList mimeTypes = KiriView::supportedOrdinaryMediaMimeTypes();

    QVERIFY(mimeTypes.contains(QStringLiteral("image/png")));
    QVERIFY(mimeTypes.contains(QStringLiteral("video/mp4")));
    QVERIFY(mimeTypes.contains(QStringLiteral("video/quicktime")));

    QStringList unique = mimeTypes;
    unique.removeDuplicates();
    QCOMPARE(mimeTypes, unique);

    QStringList sorted = mimeTypes;
    sorted.sort();
    QCOMPARE(mimeTypes, sorted);
}

void TestMediaFormatRegistry::ordinaryMediaFileNamesIncludeImagesAndDirectVideos()
{
    QVERIFY(KiriView::isSupportedOrdinaryMediaFileName(QStringLiteral("photo.PNG")));
    QVERIFY(KiriView::isSupportedOrdinaryMediaFileName(QStringLiteral("raw.CR3")));
    QVERIFY(KiriView::isSupportedOrdinaryMediaFileName(QStringLiteral("clip.MP4")));
    QVERIFY(KiriView::isSupportedOrdinaryMediaFileName(QStringLiteral("clip.m4v")));
    QVERIFY(KiriView::isSupportedOrdinaryMediaFileName(QStringLiteral("clip.mov")));

    QVERIFY(!KiriView::isSupportedOrdinaryMediaFileName(QStringLiteral("book.cbz")));
    QVERIFY(!KiriView::isSupportedOrdinaryMediaFileName(QStringLiteral("archive.zip")));
    QVERIFY(!KiriView::isSupportedOrdinaryMediaFileName(QStringLiteral(".mp4")));
}

void TestMediaFormatRegistry::directVideoFileNamesStayExposedThroughMediaRegistry()
{
    QVERIFY(KiriView::isSupportedDirectVideoFileName(QStringLiteral("clip.mp4")));
    QVERIFY(KiriView::isSupportedDirectVideoFileName(QStringLiteral("clip.M4V")));
    QVERIFY(KiriView::isSupportedDirectVideoFileName(
        QStringLiteral("zip:///path/archive.zip!/chapter/clip.MOV")));

    QVERIFY(!KiriView::isSupportedDirectVideoFileName(QStringLiteral("photo.png")));
    QVERIFY(!KiriView::isSupportedDirectVideoFileName(QStringLiteral("archive.zip")));
    QVERIFY(!KiriView::isSupportedDirectVideoFileName(QStringLiteral(".mov")));
}

void TestMediaFormatRegistry::openDialogFilterIncludesMediaAndArchives()
{
    const QStringList filters = KiriView::ordinaryMediaOpenDialogNameFilters();

    QCOMPARE(filters.size(), 2);
    QVERIFY(filters.first().contains(QStringLiteral("*.png")));
    QVERIFY(filters.first().contains(QStringLiteral("*.mp4")));
    QVERIFY(filters.first().contains(QStringLiteral("*.cbz")));
    QCOMPARE(filters.back(), QStringLiteral("All files (*)"));
}

void TestMediaFormatRegistry::desktopMimeTypesMatchSupportedOpenMimeTypes()
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

    QStringList expectedMimeTypes = KiriView::supportedOrdinaryMediaMimeTypes();
    expectedMimeTypes.append(KiriView::supportedComicBookArchiveMimeTypes());

    QCOMPARE(sortedUnique(mimeLine.split(QLatin1Char(';'), Qt::SkipEmptyParts)),
        sortedUnique(expectedMimeTypes));
}

QTEST_GUILESS_MAIN(TestMediaFormatRegistry)

#include "test_mediaformatregistry.moc"
