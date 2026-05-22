// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/mediaformatregistry.h"

#include <QObject>
#include <QStringList>
#include <QTest>

class TestMediaFormatRegistry : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ordinaryMediaExtensionsIncludeImagesAndDirectVideos();
    void ordinaryMediaFileNamesIncludeImagesAndDirectVideos();
    void directVideoFileNamesStayExposedThroughMediaRegistry();
    void openDialogFilterIncludesMediaAndArchives();
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

QTEST_GUILESS_MAIN(TestMediaFormatRegistry)

#include "test_mediaformatregistry.moc"
