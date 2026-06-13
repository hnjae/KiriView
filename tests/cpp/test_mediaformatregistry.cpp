// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/mediaformatregistry.h"

#include "archive/archiveformat.h"
#include "navigation/directmedianavigationmodel.h"

#include <QFile>
#include <QObject>
#include <QStringList>
#include <QTest>
#include <QUrl>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

QStringList sortedUnique(QStringList values)
{
    values.removeAll(QString());
    values.sort();
    values.removeDuplicates();
    return values;
}

kiriview::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl &url)
{
    return kiriview::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
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
    void directMediaUrlsClassifyImagesAndVideos();
    void stillImageDirectMediaNavigationCandidatesUseCandidateNameAndUrlIdentity();
    void openDialogFilterIncludesMediaAndArchives();
    void desktopMimeTypesMatchSupportedOpenMimeTypes();
};

void TestMediaFormatRegistry::ordinaryMediaExtensionsIncludeImagesAndDirectVideos()
{
    const QStringList extensions = kiriview::supportedOrdinaryMediaExtensions();

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
    const QStringList mimeTypes = kiriview::supportedOrdinaryMediaMimeTypes();

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
    QVERIFY(kiriview::isSupportedOrdinaryMediaFileName(QStringLiteral("photo.PNG")));
    QVERIFY(kiriview::isSupportedOrdinaryMediaFileName(QStringLiteral("raw.CR3")));
    QVERIFY(kiriview::isSupportedOrdinaryMediaFileName(QStringLiteral("clip.MP4")));
    QVERIFY(kiriview::isSupportedOrdinaryMediaFileName(QStringLiteral("clip.m4v")));
    QVERIFY(kiriview::isSupportedOrdinaryMediaFileName(QStringLiteral("clip.mov")));

    QVERIFY(!kiriview::isSupportedOrdinaryMediaFileName(QStringLiteral("book.cbz")));
    QVERIFY(!kiriview::isSupportedOrdinaryMediaFileName(QStringLiteral("archive.zip")));
    QVERIFY(!kiriview::isSupportedOrdinaryMediaFileName(QStringLiteral(".mp4")));
}

void TestMediaFormatRegistry::directVideoFileNamesStayExposedThroughMediaRegistry()
{
    QVERIFY(kiriview::isSupportedDirectVideoFileName(QStringLiteral("clip.mp4")));
    QVERIFY(kiriview::isSupportedDirectVideoFileName(QStringLiteral("clip.M4V")));
    QVERIFY(kiriview::isSupportedDirectVideoFileName(
        QStringLiteral("zip:///path/archive.zip!/chapter/clip.MOV")));

    QVERIFY(!kiriview::isSupportedDirectVideoFileName(QStringLiteral("photo.png")));
    QVERIFY(!kiriview::isSupportedDirectVideoFileName(QStringLiteral("archive.zip")));
    QVERIFY(!kiriview::isSupportedDirectVideoFileName(QStringLiteral(".mov")));
}

void TestMediaFormatRegistry::directMediaUrlsClassifyImagesAndVideos()
{
    const QUrl localVideo = localUrl(QStringLiteral("/media/clip.mp4"));
    const QUrl archiveVideo = QUrl(QStringLiteral("zip:///books/book.zip!/chapter/clip.mov"));
    const QUrl localImage = localUrl(QStringLiteral("/media/page.png"));
    const QUrl archiveImage = QUrl(QStringLiteral("zip:///books/book.zip!/chapter/page.avif"));
    const QUrl unsupported = localUrl(QStringLiteral("/media/readme.txt"));

    QVERIFY(kiriview::isSupportedDirectVideoUrl(localVideo));
    QVERIFY(kiriview::isSupportedDirectVideoUrl(archiveVideo));
    QVERIFY(!kiriview::isSupportedDirectVideoUrl(localImage));
    QVERIFY(!kiriview::isSupportedDirectVideoUrl(unsupported));

    QVERIFY(kiriview::isSupportedDirectImageUrl(localImage));
    QVERIFY(kiriview::isSupportedDirectImageUrl(archiveImage));
    QVERIFY(!kiriview::isSupportedDirectImageUrl(localVideo));
    QVERIFY(!kiriview::isSupportedDirectImageUrl(unsupported));
}

void TestMediaFormatRegistry::
    stillImageDirectMediaNavigationCandidatesUseCandidateNameAndUrlIdentity()
{
    QVERIFY(kiriview::isSupportedStillImageDirectMediaNavigationCandidate(
        kiriview::DirectMediaNavigationCandidate {
            localUrl(QStringLiteral("/media/blob.bin")),
            QStringLiteral("cover.png"),
        }));
    QVERIFY(kiriview::isSupportedStillImageDirectMediaNavigationCandidate(
        kiriview::DirectMediaNavigationCandidate {
            localUrl(QStringLiteral("/media/photo.avif")),
            QString(),
        }));
    QVERIFY(kiriview::isSupportedStillImageDirectMediaNavigationCandidate(
        kiriview::DirectMediaNavigationCandidate {
            QUrl(QStringLiteral("file:///media/download?name=cover.webp")),
            QString(),
        }));
    QVERIFY(!kiriview::isSupportedStillImageDirectMediaNavigationCandidate(
        directMediaNavigationCandidate(localUrl(QStringLiteral("/media/clip.mp4")))));
}

void TestMediaFormatRegistry::openDialogFilterIncludesMediaAndArchives()
{
    const QStringList filters = kiriview::ordinaryMediaOpenDialogNameFilters();

    QCOMPARE(filters.size(), 2);
    QVERIFY(filters.first().contains(QStringLiteral("*.png")));
    QVERIFY(filters.first().contains(QStringLiteral("*.mp4")));
    QVERIFY(filters.first().contains(QStringLiteral("*.cbz")));
    QCOMPARE(filters.back(), QStringLiteral("All files (*)"));
}

void TestMediaFormatRegistry::desktopMimeTypesMatchSupportedOpenMimeTypes()
{
    QFile desktopFile(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR "/../../org.hnjae.kiriview.desktop"));
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

    QStringList expectedMimeTypes = kiriview::supportedOrdinaryMediaMimeTypes();
    expectedMimeTypes.append(kiriview::supportedComicBookArchiveMimeTypes());

    QCOMPARE(sortedUnique(mimeLine.split(QLatin1Char(';'), Qt::SkipEmptyParts)),
        sortedUnique(expectedMimeTypes));
}

QTEST_GUILESS_MAIN(TestMediaFormatRegistry)

#include "test_mediaformatregistry.moc"
