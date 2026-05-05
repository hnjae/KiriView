// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivebackend.h"

#include "image_test_support.h"
#include "imagecontainer.h"

#include <KTar>
#include <KZip>
#include <QFile>
#include <QObject>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <optional>
#include <variant>
#include <vector>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::localUrl;

struct ArchiveEntryData {
    QString path;
    QByteArray data;
};

void writeZipArchive(const QString &path, const std::vector<ArchiveEntryData> &entries)
{
    KZip archive(path);
    QVERIFY(archive.open(QIODevice::WriteOnly));
    for (const ArchiveEntryData &entry : entries) {
        QVERIFY(archive.writeFile(entry.path, entry.data));
    }
    QVERIFY(archive.close());
}

void writeTarArchive(const QString &path, const std::vector<ArchiveEntryData> &entries)
{
    KTar archive(path);
    QVERIFY(archive.open(QIODevice::WriteOnly));
    for (const ArchiveEntryData &entry : entries) {
        QVERIFY(archive.writeFile(entry.path, entry.data));
    }
    QVERIFY(archive.close());
}

QByteArray rarArchiveData()
{
    return QByteArray::fromBase64(QByteArrayLiteral(
        "UmFyIRoHAQAzkrXlCgEFBgAFAQGAgAATXcDkLAIDC4MABIMApIMCZorKEYAAAQ5jaGFwdGVyLzAy"
        "LmpwZwoDE+AS92n+PI8KdHdvA6j5MycCAwuEAASEAKSDAn1VdviAAAEJbm90ZXMudHh0CgMT4BL3"
        "aUJ/ngpza2lw69IrPiwCAwuDAASDAKSDAvGGbHqAAAEOY2hhcHRlci8wMS5wbmcKAxPgEvdp/jyP"
        "Cm9uZR13VlEDBQQA"));
}

void writeRarArchive(const QString &path)
{
    QFile archive(path);
    QVERIFY(archive.open(QIODevice::WriteOnly));
    const QByteArray data = rarArchiveData();
    QCOMPARE(archive.write(data), static_cast<qint64>(data.size()));
    archive.close();
}

std::optional<KiriView::ArchiveDocumentLocation> archiveDocumentForPath(const QString &path)
{
    return KiriView::archiveDocumentLocationForLocalArchiveUrl(localUrl(path));
}

const KiriView::ArchiveImageCandidates *archiveImageCandidates(
    const KiriView::ArchiveImageCandidatesResult &result)
{
    return std::get_if<KiriView::ArchiveImageCandidates>(&result);
}

const KiriView::ArchiveImageData *archiveImageData(const KiriView::ArchiveImageDataResult &result)
{
    return std::get_if<KiriView::ArchiveImageData>(&result);
}

const KiriView::ArchiveError *archiveError(const KiriView::ArchiveImageCandidatesResult &result)
{
    return std::get_if<KiriView::ArchiveError>(&result);
}

const KiriView::ArchiveError *archiveDataError(const KiriView::ArchiveImageDataResult &result)
{
    return std::get_if<KiriView::ArchiveError>(&result);
}
}

class TestArchiveBackend : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void zipListingIncludesNestedSupportedImages();
    void tarListingUsesSameOrdering();
    void rarListingUsesLibArchive();
    void readingArchiveEntryReturnsOriginalBytes();
    void readingRarEntryReturnsOriginalBytes();
    void readingUrlOutsideArchiveReturnsNotFound();
    void missingEmptyAndInvalidArchivesReportExpectedResults();
};

void TestArchiveBackend::zipListingIncludesNestedSupportedImages()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.cbz"));
    writeZipArchive(archivePath,
        {
            { QStringLiteral("chapter/02.jpg"), QByteArrayLiteral("two") },
            { QStringLiteral("notes.txt"), QByteArrayLiteral("skip") },
            { QStringLiteral("chapter/01.png"), QByteArrayLiteral("one") },
        });

    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentForPath(archivePath);
    QVERIFY(archiveDocument.has_value());
    const KiriView::ArchiveImageCandidatesResult result
        = KiriView::loadArchiveDocumentImageCandidates(*archiveDocument);
    const KiriView::ArchiveImageCandidates *success = archiveImageCandidates(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->candidates.size(), std::size_t(2));
    QCOMPARE(success->candidates.at(0).name, QStringLiteral("chapter/01.png"));
    QCOMPARE(success->candidates.at(0).url,
        archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("chapter/01.png")));
    QCOMPARE(success->candidates.at(1).name, QStringLiteral("chapter/02.jpg"));
}

void TestArchiveBackend::tarListingUsesSameOrdering()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.cbt"));
    writeTarArchive(archivePath,
        {
            { QStringLiteral("pages/02.webp"), QByteArrayLiteral("two") },
            { QStringLiteral("pages/01.png"), QByteArrayLiteral("one") },
            { QStringLiteral("pages/readme.md"), QByteArrayLiteral("skip") },
        });

    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentForPath(archivePath);
    QVERIFY(archiveDocument.has_value());
    const KiriView::ArchiveImageCandidatesResult result
        = KiriView::loadArchiveDocumentImageCandidates(*archiveDocument);
    const KiriView::ArchiveImageCandidates *success = archiveImageCandidates(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->candidates.size(), std::size_t(2));
    QCOMPARE(success->candidates.at(0).name, QStringLiteral("pages/01.png"));
    QCOMPARE(success->candidates.at(1).name, QStringLiteral("pages/02.webp"));
}

void TestArchiveBackend::rarListingUsesLibArchive()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.cbr"));
    writeRarArchive(archivePath);

    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentForPath(archivePath);
    QVERIFY(archiveDocument.has_value());
    QCOMPARE(archiveDocument->rootUrl().scheme(), QStringLiteral("rar"));
    const KiriView::ArchiveImageCandidatesResult result
        = KiriView::loadArchiveDocumentImageCandidates(*archiveDocument);
    const KiriView::ArchiveImageCandidates *success = archiveImageCandidates(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->candidates.size(), std::size_t(2));
    QCOMPARE(success->candidates.at(0).name, QStringLiteral("chapter/01.png"));
    QCOMPARE(success->candidates.at(0).url,
        archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("chapter/01.png")));
    QCOMPARE(success->candidates.at(1).name, QStringLiteral("chapter/02.jpg"));
}

void TestArchiveBackend::readingArchiveEntryReturnsOriginalBytes()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.zip"));
    const QByteArray expected = QByteArrayLiteral("image-bytes");
    writeZipArchive(archivePath,
        {
            { QStringLiteral("page.png"), expected },
        });

    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentForPath(archivePath);
    QVERIFY(archiveDocument.has_value());
    const KiriView::ArchiveImageDataResult result = KiriView::loadArchiveDocumentImageData(
        *archiveDocument, archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("page.png")));
    const KiriView::ArchiveImageData *success = archiveImageData(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->data, expected);
}

void TestArchiveBackend::readingRarEntryReturnsOriginalBytes()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.rar"));
    writeRarArchive(archivePath);

    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentForPath(archivePath);
    QVERIFY(archiveDocument.has_value());
    QCOMPARE(archiveDocument->kind(), KiriView::ArchiveDocumentKind::General);
    const KiriView::ArchiveImageDataResult result
        = KiriView::loadArchiveDocumentImageData(*archiveDocument,
            archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("chapter/02.jpg")));
    const KiriView::ArchiveImageData *success = archiveImageData(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->data, QByteArrayLiteral("two"));
}

void TestArchiveBackend::readingUrlOutsideArchiveReturnsNotFound()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.zip"));
    writeZipArchive(archivePath,
        {
            { QStringLiteral("page.png"), QByteArrayLiteral("image-bytes") },
        });

    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentForPath(archivePath);
    QVERIFY(archiveDocument.has_value());
    const KiriView::ArchiveImageDataResult result = KiriView::loadArchiveDocumentImageData(
        *archiveDocument, localUrl(QStringLiteral("/outside/page.png")));
    const KiriView::ArchiveError *error = archiveDataError(result);

    QVERIFY(error != nullptr);
    QVERIFY(!error->errorString.isEmpty());
}

void TestArchiveBackend::missingEmptyAndInvalidArchivesReportExpectedResults()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const KiriView::ArchiveDocumentLocation missingArchive
        = KiriView::ArchiveDocumentLocation::fromUrls(
            localUrl(dir.filePath(QStringLiteral("missing.cbz"))),
            QUrl(QStringLiteral("zip:///missing.cbz/")), KiriView::ArchiveDocumentKind::ComicBook);
    const KiriView::ArchiveImageCandidatesResult missingResult
        = KiriView::loadArchiveDocumentImageCandidates(missingArchive);
    const KiriView::ArchiveError *missingError = archiveError(missingResult);
    QVERIFY(missingError != nullptr);
    QVERIFY(!missingError->errorString.isEmpty());

    const QString emptyArchivePath = dir.filePath(QStringLiteral("empty.cbz"));
    writeZipArchive(emptyArchivePath, {});
    const std::optional<KiriView::ArchiveDocumentLocation> emptyArchiveDocument
        = archiveDocumentForPath(emptyArchivePath);
    QVERIFY(emptyArchiveDocument.has_value());
    const KiriView::ArchiveImageCandidatesResult emptyResult
        = KiriView::loadArchiveDocumentImageCandidates(*emptyArchiveDocument);
    const KiriView::ArchiveImageCandidates *emptySuccess = archiveImageCandidates(emptyResult);
    QVERIFY(emptySuccess != nullptr);
    QVERIFY(emptySuccess->candidates.empty());

    const QString invalidArchivePath = dir.filePath(QStringLiteral("invalid.cbz"));
    QFile invalidArchive(invalidArchivePath);
    QVERIFY(invalidArchive.open(QIODevice::WriteOnly));
    QCOMPARE(invalidArchive.write(QByteArrayLiteral("not an archive")),
        qsizetype(QByteArrayLiteral("not an archive").size()));
    invalidArchive.close();

    const std::optional<KiriView::ArchiveDocumentLocation> invalidArchiveDocument
        = archiveDocumentForPath(invalidArchivePath);
    QVERIFY(invalidArchiveDocument.has_value());
    const KiriView::ArchiveImageCandidatesResult invalidResult
        = KiriView::loadArchiveDocumentImageCandidates(*invalidArchiveDocument);
    const KiriView::ArchiveError *invalidError = archiveError(invalidResult);
    QVERIFY(invalidError != nullptr);
    QVERIFY(!invalidError->errorString.isEmpty());
}

QTEST_GUILESS_MAIN(TestArchiveBackend)

#include "test_archivebackend.moc"
