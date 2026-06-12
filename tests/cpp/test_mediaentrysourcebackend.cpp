// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archive/mediaentrysourcebackend.h"

#include "archive/mediaentrysourcebackend_p.h"
#include "archive/openedcollectionthumbnailpolicy.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"

#include <K7Zip>
#include <KTar>
#include <KZip>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <optional>
#include <variant>
#include <vector>

namespace {
namespace Backend = KiriView::MediaEntrySourceBackendDetail;
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

void write7zArchive(const QString &path, const std::vector<ArchiveEntryData> &entries)
{
    K7Zip archive(path);
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

void writeFile(const QString &path, const QByteArray &data)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(data), static_cast<qint64>(data.size()));
    file.close();
}

std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollectionForPath(const QString &path)
{
    return KiriView::openedCollectionScopeLocationForLocalArchiveUrl(localUrl(path));
}

const KiriView::MediaEntrySourceCandidates *mediaEntrySourceCandidates(
    const KiriView::MediaEntrySourceCandidatesResult &result)
{
    return std::get_if<KiriView::MediaEntrySourceCandidates>(&result);
}

const KiriView::MediaEntrySourceImageData *mediaEntrySourceImageData(
    const KiriView::MediaEntrySourceImageDataResult &result)
{
    return std::get_if<KiriView::MediaEntrySourceImageData>(&result);
}

const KiriView::MediaEntrySourceThumbnailMetadata *mediaEntrySourceThumbnailMetadata(
    const KiriView::MediaEntrySourceThumbnailMetadataResult &result)
{
    return std::get_if<KiriView::MediaEntrySourceThumbnailMetadata>(&result);
}

const KiriView::MediaEntrySourceError *mediaEntrySourceError(
    const KiriView::MediaEntrySourceCandidatesResult &result)
{
    return std::get_if<KiriView::MediaEntrySourceError>(&result);
}

const KiriView::MediaEntrySourceError *mediaEntrySourceDataError(
    const KiriView::MediaEntrySourceImageDataResult &result)
{
    return std::get_if<KiriView::MediaEntrySourceError>(&result);
}

class CandidateSnapshotSource final : public Backend::MediaEntrySourceWithCandidateSnapshot
{
public:
    explicit CandidateSnapshotSource(std::vector<KiriView::ImageDocumentPageCandidate> candidates)
        : Backend::MediaEntrySourceWithCandidateSnapshot(std::move(candidates))
    {
    }

    KiriView::MediaEntrySourceImageDataResult loadImageData(const QUrl &) override
    {
        return KiriView::MediaEntrySourceImageData {};
    }
};
}

class TestMediaEntrySourceBackend : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void zipListingIncludesNestedSupportedMedia();
    void directoryListingIncludesNestedSupportedMedia();
    void tarListingUsesSameOrdering();
    void rarListingUsesLibArchive();
    void readingArchiveEntryReturnsOriginalBytes();
    void readingDirectoryEntryReturnsOriginalBytes();
    void readingRarEntryReturnsOriginalBytes();
    void standaloneHelpersMatchMediaEntrySourceResults();
    void candidateSnapshotSourcesOwnSortedDefensiveListing();
    void kArchiveMediaEntrySourceListsAndReadsEntries();
    void directoryCollectionMediaEntrySourceListsAndReadsEntries();
    void libArchiveMediaEntrySourceScansOnceAndServesRandomReads();
    void libArchiveMediaEntrySourceReadsFromHeldFileDescriptorAfterPathRemoval();
    void openedCollectionThumbnailPolicyAllowsZipBackedImagesOnly();
    void cbzImageEntryReturnsThumbnailMetadata();
    void genericZipImageEntryReturnsThumbnailMetadata();
    void cb7ImageEntryDoesNotReturnThumbnailMetadata();
    void unsupportedCollectionEntriesDoNotReturnThumbnailMetadata();
    void readingUrlOutsideArchiveReturnsNotFound();
    void missingEmptyAndInvalidArchivesReportExpectedResults();
};

void TestMediaEntrySourceBackend::zipListingIncludesNestedSupportedMedia()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.cbz"));
    writeZipArchive(archivePath,
        {
            { QStringLiteral("chapter/02.jpg"), QByteArrayLiteral("two") },
            { QStringLiteral("notes.txt"), QByteArrayLiteral("skip") },
            { QStringLiteral("chapter/03.mp4"), QByteArrayLiteral("video") },
            { QStringLiteral("chapter/01.png"), QByteArrayLiteral("one") },
        });

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    const KiriView::MediaEntrySourceCandidatesResult result
        = KiriView::loadMediaEntrySourceCandidates(*archiveCollection);
    const KiriView::MediaEntrySourceCandidates *success = mediaEntrySourceCandidates(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->candidates.size(), std::size_t(3));
    QCOMPARE(success->candidates.at(0).name, QStringLiteral("chapter/01.png"));
    QCOMPARE(success->candidates.at(0).url,
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    QCOMPARE(success->candidates.at(0).kind, KiriView::ImageDocumentPageKind::Image);
    QCOMPARE(success->candidates.at(1).name, QStringLiteral("chapter/02.jpg"));
    QCOMPARE(success->candidates.at(1).kind, KiriView::ImageDocumentPageKind::Image);
    QCOMPARE(success->candidates.at(2).name, QStringLiteral("chapter/03.mp4"));
    QCOMPARE(success->candidates.at(2).kind, KiriView::ImageDocumentPageKind::Video);
}

void TestMediaEntrySourceBackend::directoryListingIncludesNestedSupportedMedia()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QDir root(dir.path());
    QVERIFY(root.mkpath(QStringLiteral("chapter")));
    writeFile(dir.filePath(QStringLiteral("chapter/02.jpg")), QByteArrayLiteral("two"));
    writeFile(dir.filePath(QStringLiteral("notes.txt")), QByteArrayLiteral("skip"));
    writeFile(dir.filePath(QStringLiteral("chapter/03.m4v")), QByteArrayLiteral("video"));
    writeFile(dir.filePath(QStringLiteral("chapter/01.png")), QByteArrayLiteral("one"));

    const std::optional<KiriView::OpenedCollectionScopeLocation> directoryCollection
        = KiriView::openedCollectionScopeLocationForDirectlyOpenedLocalUrl(localUrl(dir.path()));
    QVERIFY(directoryCollection.has_value());
    QCOMPARE(directoryCollection->kind(), KiriView::OpenedCollectionScopeKind::Directory);
    const KiriView::MediaEntrySourceCandidatesResult result
        = KiriView::loadMediaEntrySourceCandidates(*directoryCollection);
    const KiriView::MediaEntrySourceCandidates *success = mediaEntrySourceCandidates(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->candidates.size(), std::size_t(3));
    QCOMPARE(success->candidates.at(0).name, QStringLiteral("chapter/01.png"));
    QCOMPARE(success->candidates.at(0).url,
        archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    QCOMPARE(success->candidates.at(0).kind, KiriView::ImageDocumentPageKind::Image);
    QCOMPARE(success->candidates.at(1).name, QStringLiteral("chapter/02.jpg"));
    QCOMPARE(success->candidates.at(1).kind, KiriView::ImageDocumentPageKind::Image);
    QCOMPARE(success->candidates.at(2).name, QStringLiteral("chapter/03.m4v"));
    QCOMPARE(success->candidates.at(2).kind, KiriView::ImageDocumentPageKind::Video);
}

void TestMediaEntrySourceBackend::tarListingUsesSameOrdering()
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

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    const KiriView::MediaEntrySourceCandidatesResult result
        = KiriView::loadMediaEntrySourceCandidates(*archiveCollection);
    const KiriView::MediaEntrySourceCandidates *success = mediaEntrySourceCandidates(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->candidates.size(), std::size_t(2));
    QCOMPARE(success->candidates.at(0).name, QStringLiteral("pages/01.png"));
    QCOMPARE(success->candidates.at(1).name, QStringLiteral("pages/02.webp"));
}

void TestMediaEntrySourceBackend::rarListingUsesLibArchive()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.cbr"));
    writeRarArchive(archivePath);

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    QCOMPARE(archiveCollection->rootUrl().scheme(), QStringLiteral("rar"));
    const KiriView::MediaEntrySourceCandidatesResult result
        = KiriView::loadMediaEntrySourceCandidates(*archiveCollection);
    const KiriView::MediaEntrySourceCandidates *success = mediaEntrySourceCandidates(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->candidates.size(), std::size_t(2));
    QCOMPARE(success->candidates.at(0).name, QStringLiteral("chapter/01.png"));
    QCOMPARE(success->candidates.at(0).url,
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    QCOMPARE(success->candidates.at(1).name, QStringLiteral("chapter/02.jpg"));
}

void TestMediaEntrySourceBackend::readingArchiveEntryReturnsOriginalBytes()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.zip"));
    const QByteArray expected = QByteArrayLiteral("image-bytes");
    writeZipArchive(archivePath,
        {
            { QStringLiteral("page.png"), expected },
        });

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    const KiriView::MediaEntrySourceImageDataResult result
        = KiriView::loadMediaEntrySourceImageData(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("page.png")));
    const KiriView::MediaEntrySourceImageData *success = mediaEntrySourceImageData(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->data, expected);
}

void TestMediaEntrySourceBackend::readingDirectoryEntryReturnsOriginalBytes()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QDir root(dir.path());
    QVERIFY(root.mkpath(QStringLiteral("pages")));
    const QByteArray expected = QByteArrayLiteral("image-bytes");
    writeFile(dir.filePath(QStringLiteral("pages/page.png")), expected);

    const std::optional<KiriView::OpenedCollectionScopeLocation> directoryCollection
        = KiriView::openedCollectionScopeLocationForDirectlyOpenedLocalUrl(localUrl(dir.path()));
    QVERIFY(directoryCollection.has_value());
    const KiriView::MediaEntrySourceImageDataResult result
        = KiriView::loadMediaEntrySourceImageData(*directoryCollection,
            archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("pages/page.png")));
    const KiriView::MediaEntrySourceImageData *success = mediaEntrySourceImageData(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->data, expected);
}

void TestMediaEntrySourceBackend::readingRarEntryReturnsOriginalBytes()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.rar"));
    writeRarArchive(archivePath);

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    QCOMPARE(archiveCollection->kind(), KiriView::OpenedCollectionScopeKind::GeneralArchive);
    const KiriView::MediaEntrySourceImageDataResult result
        = KiriView::loadMediaEntrySourceImageData(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/02.jpg")));
    const KiriView::MediaEntrySourceImageData *success = mediaEntrySourceImageData(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->data, QByteArrayLiteral("two"));
}

void TestMediaEntrySourceBackend::standaloneHelpersMatchMediaEntrySourceResults()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.zip"));
    writeZipArchive(archivePath,
        {
            { QStringLiteral("pages/02.jpg"), QByteArrayLiteral("two") },
            { QStringLiteral("pages/01.png"), QByteArrayLiteral("one") },
        });

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    KiriView::MediaEntrySourceOpenResult opened
        = KiriView::openMediaEntrySource(*archiveCollection);
    auto *source = std::get_if<KiriView::MediaEntrySourcePtr>(&opened);
    QVERIFY(source != nullptr);
    QVERIFY(*source != nullptr);

    const KiriView::MediaEntrySourceCandidatesResult standaloneCandidatesResult
        = KiriView::loadMediaEntrySourceCandidates(*archiveCollection);
    const KiriView::MediaEntrySourceCandidatesResult sourceCandidatesResult
        = (*source)->loadImageDocumentPageCandidates();
    const KiriView::MediaEntrySourceCandidates *standaloneCandidates
        = mediaEntrySourceCandidates(standaloneCandidatesResult);
    const KiriView::MediaEntrySourceCandidates *sourceCandidates
        = mediaEntrySourceCandidates(sourceCandidatesResult);
    QVERIFY(standaloneCandidates != nullptr);
    QVERIFY(sourceCandidates != nullptr);
    QCOMPARE(standaloneCandidates->candidates.size(), sourceCandidates->candidates.size());
    QCOMPARE(standaloneCandidates->candidates.at(0).name, sourceCandidates->candidates.at(0).name);
    QCOMPARE(standaloneCandidates->candidates.at(1).name, sourceCandidates->candidates.at(1).name);

    const QUrl imageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/01.png"));
    const KiriView::MediaEntrySourceImageDataResult standaloneDataResult
        = KiriView::loadMediaEntrySourceImageData(*archiveCollection, imageUrl);
    const KiriView::MediaEntrySourceImageDataResult sourceDataResult
        = (*source)->loadImageData(imageUrl);
    const KiriView::MediaEntrySourceImageData *standaloneData
        = mediaEntrySourceImageData(standaloneDataResult);
    const KiriView::MediaEntrySourceImageData *sourceData
        = mediaEntrySourceImageData(sourceDataResult);
    QVERIFY(standaloneData != nullptr);
    QVERIFY(sourceData != nullptr);
    QCOMPARE(standaloneData->data, sourceData->data);
}

void TestMediaEntrySourceBackend::candidateSnapshotSourcesOwnSortedDefensiveListing()
{
    const QUrl archiveRootUrl(QStringLiteral("zip:///books/book.cbz/"));
    CandidateSnapshotSource source({
        KiriView::ImageDocumentPageCandidate {
            archivePageUrl(archiveRootUrl, QStringLiteral("pages/02.jpg")),
            QStringLiteral("pages/02.jpg"),
        },
        KiriView::ImageDocumentPageCandidate {
            archivePageUrl(archiveRootUrl, QStringLiteral("pages/01.png")),
            QStringLiteral("pages/01.png"),
        },
    });

    KiriView::MediaEntrySourceCandidatesResult firstResult
        = source.loadImageDocumentPageCandidates();
    auto *first = std::get_if<KiriView::MediaEntrySourceCandidates>(&firstResult);
    QVERIFY(first != nullptr);
    QCOMPARE(first->candidates.size(), std::size_t(2));
    QCOMPARE(first->candidates.at(0).name, QStringLiteral("pages/01.png"));
    QCOMPARE(first->candidates.at(1).name, QStringLiteral("pages/02.jpg"));

    first->candidates.clear();

    const KiriView::MediaEntrySourceCandidatesResult secondResult
        = source.loadImageDocumentPageCandidates();
    const KiriView::MediaEntrySourceCandidates *second = mediaEntrySourceCandidates(secondResult);
    QVERIFY(second != nullptr);
    QCOMPARE(second->candidates.size(), std::size_t(2));
    QCOMPARE(second->candidates.at(0).name, QStringLiteral("pages/01.png"));
    QCOMPARE(second->candidates.at(1).name, QStringLiteral("pages/02.jpg"));
}

void TestMediaEntrySourceBackend::kArchiveMediaEntrySourceListsAndReadsEntries()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.zip"));
    writeZipArchive(archivePath,
        {
            { QStringLiteral("pages/02.jpg"), QByteArrayLiteral("two") },
            { QStringLiteral("pages/01.png"), QByteArrayLiteral("one") },
        });

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    KiriView::MediaEntrySourceOpenResult opened
        = KiriView::openMediaEntrySource(*archiveCollection);
    auto *source = std::get_if<KiriView::MediaEntrySourcePtr>(&opened);
    QVERIFY(source != nullptr);
    QVERIFY(*source != nullptr);

    const KiriView::MediaEntrySourceCandidatesResult candidatesResult
        = (*source)->loadImageDocumentPageCandidates();
    const KiriView::MediaEntrySourceCandidates *candidates
        = mediaEntrySourceCandidates(candidatesResult);
    QVERIFY(candidates != nullptr);
    QCOMPARE(candidates->candidates.size(), std::size_t(2));
    QCOMPARE(candidates->candidates.at(0).name, QStringLiteral("pages/01.png"));

    const KiriView::MediaEntrySourceImageDataResult secondDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/02.jpg")));
    const KiriView::MediaEntrySourceImageData *secondData
        = mediaEntrySourceImageData(secondDataResult);
    QVERIFY(secondData != nullptr);
    QCOMPARE(secondData->data, QByteArrayLiteral("two"));

    const KiriView::MediaEntrySourceImageDataResult firstDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/01.png")));
    const KiriView::MediaEntrySourceImageData *firstData
        = mediaEntrySourceImageData(firstDataResult);
    QVERIFY(firstData != nullptr);
    QCOMPARE(firstData->data, QByteArrayLiteral("one"));
}

void TestMediaEntrySourceBackend::directoryCollectionMediaEntrySourceListsAndReadsEntries()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QDir root(dir.path());
    QVERIFY(root.mkpath(QStringLiteral("pages")));
    writeFile(dir.filePath(QStringLiteral("pages/02.jpg")), QByteArrayLiteral("two"));
    writeFile(dir.filePath(QStringLiteral("pages/01.png")), QByteArrayLiteral("one"));

    const std::optional<KiriView::OpenedCollectionScopeLocation> directoryCollection
        = KiriView::openedCollectionScopeLocationForDirectlyOpenedLocalUrl(localUrl(dir.path()));
    QVERIFY(directoryCollection.has_value());
    KiriView::MediaEntrySourceOpenResult opened
        = KiriView::openMediaEntrySource(*directoryCollection);
    auto *source = std::get_if<KiriView::MediaEntrySourcePtr>(&opened);
    QVERIFY(source != nullptr);
    QVERIFY(*source != nullptr);

    const KiriView::MediaEntrySourceCandidatesResult candidatesResult
        = (*source)->loadImageDocumentPageCandidates();
    const KiriView::MediaEntrySourceCandidates *candidates
        = mediaEntrySourceCandidates(candidatesResult);
    QVERIFY(candidates != nullptr);
    QCOMPARE(candidates->candidates.size(), std::size_t(2));
    QCOMPARE(candidates->candidates.at(0).name, QStringLiteral("pages/01.png"));

    const KiriView::MediaEntrySourceImageDataResult secondDataResult = (*source)->loadImageData(
        archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("pages/02.jpg")));
    const KiriView::MediaEntrySourceImageData *secondData
        = mediaEntrySourceImageData(secondDataResult);
    QVERIFY(secondData != nullptr);
    QCOMPARE(secondData->data, QByteArrayLiteral("two"));
}

void TestMediaEntrySourceBackend::libArchiveMediaEntrySourceScansOnceAndServesRandomReads()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.rar"));
    writeRarArchive(archivePath);

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    KiriView::MediaEntrySourceOpenResult opened
        = KiriView::openMediaEntrySource(*archiveCollection);
    auto *source = std::get_if<KiriView::MediaEntrySourcePtr>(&opened);
    QVERIFY(source != nullptr);
    QVERIFY(*source != nullptr);

    const KiriView::MediaEntrySourceCandidatesResult candidatesResult
        = (*source)->loadImageDocumentPageCandidates();
    const KiriView::MediaEntrySourceCandidates *candidates
        = mediaEntrySourceCandidates(candidatesResult);
    QVERIFY(candidates != nullptr);
    QCOMPARE(candidates->candidates.size(), std::size_t(2));
    QCOMPARE(candidates->candidates.at(0).name, QStringLiteral("chapter/01.png"));
    QCOMPARE(candidates->candidates.at(1).name, QStringLiteral("chapter/02.jpg"));

    // The fixture stores 02.jpg before 01.png, so this read order exercises replay.
    const KiriView::MediaEntrySourceImageDataResult firstDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    const KiriView::MediaEntrySourceImageData *firstData
        = mediaEntrySourceImageData(firstDataResult);
    QVERIFY(firstData != nullptr);
    QCOMPARE(firstData->data, QByteArrayLiteral("one"));

    const KiriView::MediaEntrySourceImageDataResult secondDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/02.jpg")));
    const KiriView::MediaEntrySourceImageData *secondData
        = mediaEntrySourceImageData(secondDataResult);
    QVERIFY(secondData != nullptr);
    QCOMPARE(secondData->data, QByteArrayLiteral("two"));
}

void TestMediaEntrySourceBackend::
    libArchiveMediaEntrySourceReadsFromHeldFileDescriptorAfterPathRemoval()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.rar"));
    writeRarArchive(archivePath);

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    KiriView::MediaEntrySourceOpenResult opened
        = KiriView::openMediaEntrySource(*archiveCollection);
    auto *source = std::get_if<KiriView::MediaEntrySourcePtr>(&opened);
    QVERIFY(source != nullptr);
    QVERIFY(*source != nullptr);

    const KiriView::MediaEntrySourceCandidatesResult candidatesResult
        = (*source)->loadImageDocumentPageCandidates();
    const KiriView::MediaEntrySourceCandidates *candidates
        = mediaEntrySourceCandidates(candidatesResult);
    QVERIFY(candidates != nullptr);
    QCOMPARE(candidates->candidates.size(), std::size_t(2));

    QVERIFY(QFile::remove(archivePath));

    const KiriView::MediaEntrySourceImageDataResult firstDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    const KiriView::MediaEntrySourceImageData *firstData
        = mediaEntrySourceImageData(firstDataResult);
    QVERIFY(firstData != nullptr);
    QCOMPARE(firstData->data, QByteArrayLiteral("one"));

    const KiriView::MediaEntrySourceImageDataResult secondDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/02.jpg")));
    const KiriView::MediaEntrySourceImageData *secondData
        = mediaEntrySourceImageData(secondDataResult);
    QVERIFY(secondData != nullptr);
    QCOMPARE(secondData->data, QByteArrayLiteral("two"));

    const KiriView::MediaEntrySourceImageDataResult standaloneResult
        = KiriView::loadMediaEntrySourceImageData(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    const KiriView::MediaEntrySourceError *standaloneError
        = mediaEntrySourceDataError(standaloneResult);
    QVERIFY(standaloneError != nullptr);
    QVERIFY(!standaloneError->errorString.isEmpty());
}

void TestMediaEntrySourceBackend::openedCollectionThumbnailPolicyAllowsZipBackedImagesOnly()
{
    using Kind = KiriView::OpenedCollectionThumbnailSourcePlanKind;

    const KiriView::OpenedCollectionScopeLocation cbzScope
        = KiriView::OpenedCollectionScopeLocation::fromUrls(
            localUrl(QStringLiteral("/books/book.cbz")),
            QUrl(QStringLiteral("zip:///books/book.cbz/")),
            KiriView::OpenedCollectionScopeKind::ComicBookArchive);
    const QUrl cbzPage = archivePageUrl(cbzScope.rootUrl(), QStringLiteral("pages/001.png"));
    QCOMPARE(KiriView::openedCollectionThumbnailSourcePlan(
                 cbzScope, cbzPage, KiriView::ImageDocumentPageKind::Image)
                 .kind,
        Kind::CacheableOpenedCollectionEntry);
    QCOMPARE(KiriView::openedCollectionThumbnailSourcePlan(
                 cbzScope, cbzPage, KiriView::ImageDocumentPageKind::Image)
                 .openedCollectionScope,
        cbzScope);
    QCOMPARE(KiriView::openedCollectionThumbnailSourcePlan(
                 cbzScope, cbzPage, KiriView::ImageDocumentPageKind::Video)
                 .kind,
        Kind::Unsupported);

    const KiriView::OpenedCollectionScopeLocation cb7Scope
        = KiriView::OpenedCollectionScopeLocation::fromUrls(
            localUrl(QStringLiteral("/books/book.cb7")),
            QUrl(QStringLiteral("sevenz:///books/book.cb7/")),
            KiriView::OpenedCollectionScopeKind::ComicBookArchive);
    QCOMPARE(KiriView::openedCollectionThumbnailSourcePlan(cb7Scope,
                 archivePageUrl(cb7Scope.rootUrl(), QStringLiteral("pages/001.jpg")),
                 KiriView::ImageDocumentPageKind::Image)
                 .kind,
        Kind::Unsupported);

    const KiriView::OpenedCollectionScopeLocation cbtScope
        = KiriView::OpenedCollectionScopeLocation::fromUrls(
            localUrl(QStringLiteral("/books/book.cbt")),
            QUrl(QStringLiteral("tar:///books/book.cbt/")),
            KiriView::OpenedCollectionScopeKind::ComicBookArchive);
    QCOMPARE(KiriView::openedCollectionThumbnailSourcePlan(cbtScope,
                 archivePageUrl(cbtScope.rootUrl(), QStringLiteral("pages/001.png")),
                 KiriView::ImageDocumentPageKind::Image)
                 .kind,
        Kind::Unsupported);

    const KiriView::OpenedCollectionScopeLocation genericZipScope
        = KiriView::OpenedCollectionScopeLocation::fromUrls(
            localUrl(QStringLiteral("/books/book.zip")),
            QUrl(QStringLiteral("zip:///books/book.zip/")),
            KiriView::OpenedCollectionScopeKind::GeneralArchive);
    QCOMPARE(KiriView::openedCollectionThumbnailSourcePlan(genericZipScope,
                 archivePageUrl(genericZipScope.rootUrl(), QStringLiteral("pages/001.png")),
                 KiriView::ImageDocumentPageKind::Image)
                 .kind,
        Kind::CacheableOpenedCollectionEntry);

    const KiriView::OpenedCollectionScopeLocation directoryScope
        = KiriView::OpenedCollectionScopeLocation::fromUrls(localUrl(QStringLiteral("/books/dir")),
            localUrl(QStringLiteral("/books/dir/")),
            KiriView::OpenedCollectionScopeKind::Directory);
    QCOMPARE(KiriView::openedCollectionThumbnailSourcePlan(directoryScope,
                 archivePageUrl(directoryScope.rootUrl(), QStringLiteral("pages/001.png")),
                 KiriView::ImageDocumentPageKind::Image)
                 .kind,
        Kind::Unsupported);

    QCOMPARE(KiriView::openedCollectionThumbnailSourcePlan(cbzScope,
                 archivePageUrl(QUrl(QStringLiteral("zip:///books/other.cbz/")),
                     QStringLiteral("pages/001.png")),
                 KiriView::ImageDocumentPageKind::Image)
                 .kind,
        Kind::Unsupported);
}

void TestMediaEntrySourceBackend::cbzImageEntryReturnsThumbnailMetadata()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.cbz"));
    writeZipArchive(archivePath,
        {
            { QStringLiteral("pages/01.png"), QByteArrayLiteral("one") },
        });

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    const KiriView::MediaEntrySourceThumbnailMetadataResult result
        = KiriView::loadMediaEntrySourceThumbnailMetadata(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/01.png")));
    const KiriView::MediaEntrySourceThumbnailMetadata *metadata
        = mediaEntrySourceThumbnailMetadata(result);

    QVERIFY(metadata != nullptr);
    QCOMPARE(metadata->checksum.algorithm, KiriView::MediaEntryContentChecksumAlgorithm::Crc32);
    QCOMPARE(metadata->checksum.value, quint64(0x7a6c86f1));
    QCOMPARE(metadata->uncompressedSize, qint64(3));
}

void TestMediaEntrySourceBackend::genericZipImageEntryReturnsThumbnailMetadata()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.zip"));
    writeZipArchive(archivePath,
        {
            { QStringLiteral("pages/01.png"), QByteArrayLiteral("one") },
        });

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    const KiriView::MediaEntrySourceThumbnailMetadataResult result
        = KiriView::loadMediaEntrySourceThumbnailMetadata(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/01.png")));
    const KiriView::MediaEntrySourceThumbnailMetadata *metadata
        = mediaEntrySourceThumbnailMetadata(result);

    QVERIFY(metadata != nullptr);
    QCOMPARE(metadata->checksum.algorithm, KiriView::MediaEntryContentChecksumAlgorithm::Crc32);
    QCOMPARE(metadata->checksum.value, quint64(0x7a6c86f1));
    QCOMPARE(metadata->uncompressedSize, qint64(3));
}

void TestMediaEntrySourceBackend::cb7ImageEntryDoesNotReturnThumbnailMetadata()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.cb7"));
    write7zArchive(archivePath,
        {
            { QStringLiteral("pages/01.png"), QByteArrayLiteral("one") },
        });

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    const KiriView::MediaEntrySourceThumbnailMetadataResult result
        = KiriView::loadMediaEntrySourceThumbnailMetadata(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/01.png")));
    QVERIFY(std::get_if<KiriView::MediaEntrySourceError>(&result) != nullptr);
}

void TestMediaEntrySourceBackend::unsupportedCollectionEntriesDoNotReturnThumbnailMetadata()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString cbtPath = dir.filePath(QStringLiteral("book.cbt"));
    const QString cbrPath = dir.filePath(QStringLiteral("book.cbr"));
    writeTarArchive(cbtPath,
        {
            { QStringLiteral("pages/01.png"), QByteArrayLiteral("one") },
        });
    writeRarArchive(cbrPath);

    const std::optional<KiriView::OpenedCollectionScopeLocation> cbtCollection
        = archiveCollectionForPath(cbtPath);
    const std::optional<KiriView::OpenedCollectionScopeLocation> cbrCollection
        = archiveCollectionForPath(cbrPath);
    QVERIFY(cbtCollection.has_value());
    QVERIFY(cbrCollection.has_value());

    const KiriView::MediaEntrySourceThumbnailMetadataResult cbtResult
        = KiriView::loadMediaEntrySourceThumbnailMetadata(*cbtCollection,
            archivePageUrl(cbtCollection->rootUrl(), QStringLiteral("pages/01.png")));
    QVERIFY(std::get_if<KiriView::MediaEntrySourceError>(&cbtResult) != nullptr);

    const KiriView::MediaEntrySourceThumbnailMetadataResult cbrResult
        = KiriView::loadMediaEntrySourceThumbnailMetadata(*cbrCollection,
            archivePageUrl(cbrCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    QVERIFY(std::get_if<KiriView::MediaEntrySourceError>(&cbrResult) != nullptr);

    const std::optional<KiriView::OpenedCollectionScopeLocation> directoryCollection
        = KiriView::openedCollectionScopeLocationForDirectlyOpenedLocalUrl(localUrl(dir.path()));
    QVERIFY(directoryCollection.has_value());
    const KiriView::MediaEntrySourceThumbnailMetadataResult directoryResult
        = KiriView::loadMediaEntrySourceThumbnailMetadata(*directoryCollection,
            archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("pages/01.png")));
    QVERIFY(std::get_if<KiriView::MediaEntrySourceError>(&directoryResult) != nullptr);

    const QString cbzPath = dir.filePath(QStringLiteral("book.cbz"));
    writeZipArchive(cbzPath,
        {
            { QStringLiteral("pages/clip.mp4"), QByteArrayLiteral("video") },
        });
    const std::optional<KiriView::OpenedCollectionScopeLocation> cbzCollection
        = archiveCollectionForPath(cbzPath);
    QVERIFY(cbzCollection.has_value());
    const KiriView::MediaEntrySourceThumbnailMetadataResult videoResult
        = KiriView::loadMediaEntrySourceThumbnailMetadata(*cbzCollection,
            archivePageUrl(cbzCollection->rootUrl(), QStringLiteral("pages/clip.mp4")));
    QVERIFY(std::get_if<KiriView::MediaEntrySourceError>(&videoResult) != nullptr);
}

void TestMediaEntrySourceBackend::readingUrlOutsideArchiveReturnsNotFound()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.zip"));
    writeZipArchive(archivePath,
        {
            { QStringLiteral("page.png"), QByteArrayLiteral("image-bytes") },
        });

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    const KiriView::MediaEntrySourceImageDataResult result
        = KiriView::loadMediaEntrySourceImageData(
            *archiveCollection, localUrl(QStringLiteral("/outside/page.png")));
    const KiriView::MediaEntrySourceError *error = mediaEntrySourceDataError(result);

    QVERIFY(error != nullptr);
    QVERIFY(!error->errorString.isEmpty());
}

void TestMediaEntrySourceBackend::missingEmptyAndInvalidArchivesReportExpectedResults()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const KiriView::OpenedCollectionScopeLocation missingArchive
        = KiriView::OpenedCollectionScopeLocation::fromUrls(
            localUrl(dir.filePath(QStringLiteral("missing.cbz"))),
            QUrl(QStringLiteral("zip:///missing.cbz/")),
            KiriView::OpenedCollectionScopeKind::ComicBookArchive);
    const KiriView::MediaEntrySourceCandidatesResult missingResult
        = KiriView::loadMediaEntrySourceCandidates(missingArchive);
    const KiriView::MediaEntrySourceError *missingError = mediaEntrySourceError(missingResult);
    QVERIFY(missingError != nullptr);
    QVERIFY(!missingError->errorString.isEmpty());

    const QString emptyArchivePath = dir.filePath(QStringLiteral("empty.cbz"));
    writeZipArchive(emptyArchivePath, {});
    const std::optional<KiriView::OpenedCollectionScopeLocation> emptyArchiveCollection
        = archiveCollectionForPath(emptyArchivePath);
    QVERIFY(emptyArchiveCollection.has_value());
    const KiriView::MediaEntrySourceCandidatesResult emptyResult
        = KiriView::loadMediaEntrySourceCandidates(*emptyArchiveCollection);
    const KiriView::MediaEntrySourceCandidates *emptySuccess
        = mediaEntrySourceCandidates(emptyResult);
    QVERIFY(emptySuccess != nullptr);
    QVERIFY(emptySuccess->candidates.empty());

    const QString invalidArchivePath = dir.filePath(QStringLiteral("invalid.cbz"));
    QFile invalidArchive(invalidArchivePath);
    QVERIFY(invalidArchive.open(QIODevice::WriteOnly));
    QCOMPARE(invalidArchive.write(QByteArrayLiteral("not an archive")),
        qsizetype(QByteArrayLiteral("not an archive").size()));
    invalidArchive.close();

    const std::optional<KiriView::OpenedCollectionScopeLocation> invalidArchiveCollection
        = archiveCollectionForPath(invalidArchivePath);
    QVERIFY(invalidArchiveCollection.has_value());
    const KiriView::MediaEntrySourceCandidatesResult invalidResult
        = KiriView::loadMediaEntrySourceCandidates(*invalidArchiveCollection);
    const KiriView::MediaEntrySourceError *invalidError = mediaEntrySourceError(invalidResult);
    QVERIFY(invalidError != nullptr);
    QVERIFY(!invalidError->errorString.isEmpty());
}

QTEST_GUILESS_MAIN(TestMediaEntrySourceBackend)

#include "test_mediaentrysourcebackend.moc"
