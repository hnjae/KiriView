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
namespace Backend = kiriview::MediaEntrySourceBackendDetail;
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::localUrl;

struct ArchiveEntryData
{
    QString path;
    QByteArray data;
};

void writeZipArchive(const QString& path, const std::vector<ArchiveEntryData>& entries)
{
    KZip archive(path);
    QVERIFY(archive.open(QIODevice::WriteOnly));
    for (const ArchiveEntryData& entry : entries) {
        QVERIFY(archive.writeFile(entry.path, entry.data));
    }
    QVERIFY(archive.close());
}

void writeZipArchiveWithCompression(const QString& path,
    const std::vector<ArchiveEntryData>& entries, KZip::Compression compression)
{
    KZip archive(path);
    QVERIFY(archive.open(QIODevice::WriteOnly));
    archive.setCompression(compression);
    for (const ArchiveEntryData& entry : entries) {
        QVERIFY(archive.writeFile(entry.path, entry.data));
    }
    QVERIFY(archive.close());
}

void writeTarArchive(const QString& path, const std::vector<ArchiveEntryData>& entries)
{
    KTar archive(path);
    QVERIFY(archive.open(QIODevice::WriteOnly));
    for (const ArchiveEntryData& entry : entries) {
        QVERIFY(archive.writeFile(entry.path, entry.data));
    }
    QVERIFY(archive.close());
}

void write7zArchive(const QString& path, const std::vector<ArchiveEntryData>& entries)
{
    K7Zip archive(path);
    QVERIFY(archive.open(QIODevice::WriteOnly));
    for (const ArchiveEntryData& entry : entries) {
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

void writeRarArchive(const QString& path)
{
    QFile archive(path);
    QVERIFY(archive.open(QIODevice::WriteOnly));
    const QByteArray data = rarArchiveData();
    QCOMPARE(archive.write(data), static_cast<qint64>(data.size()));
    archive.close();
}

void writeFile(const QString& path, const QByteArray& data)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(data), static_cast<qint64>(data.size()));
    file.close();
}

std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollectionForPath(const QString& path)
{
    return kiriview::openedCollectionScopeLocationForLocalArchiveUrl(localUrl(path));
}

const kiriview::MediaEntrySourceCandidates* mediaEntrySourceCandidates(
    const kiriview::MediaEntrySourceCandidatesResult& result)
{
    return std::get_if<kiriview::MediaEntrySourceCandidates>(&result);
}

const kiriview::MediaEntrySourceImageData* mediaEntrySourceImageData(
    const kiriview::MediaEntrySourceImageDataResult& result)
{
    return std::get_if<kiriview::MediaEntrySourceImageData>(&result);
}

const kiriview::MediaEntrySourceThumbnailMetadata* mediaEntrySourceThumbnailMetadata(
    const kiriview::MediaEntrySourceThumbnailMetadataResult& result)
{
    return std::get_if<kiriview::MediaEntrySourceThumbnailMetadata>(&result);
}

kiriview::MediaEntrySourceVideoPlaybackDevice* mediaEntrySourceVideoPlaybackDevice(
    kiriview::MediaEntrySourceVideoPlaybackDeviceResult& result)
{
    return std::get_if<kiriview::MediaEntrySourceVideoPlaybackDevice>(&result);
}

const kiriview::MediaEntrySourceError* mediaEntrySourceError(
    const kiriview::MediaEntrySourceCandidatesResult& result)
{
    return std::get_if<kiriview::MediaEntrySourceError>(&result);
}

const kiriview::MediaEntrySourceError* mediaEntrySourceDataError(
    const kiriview::MediaEntrySourceImageDataResult& result)
{
    return std::get_if<kiriview::MediaEntrySourceError>(&result);
}

class CandidateSnapshotSource final : public Backend::MediaEntrySourceWithCandidateSnapshot
{
public:
    explicit CandidateSnapshotSource(std::vector<kiriview::ImageDocumentPageCandidate> candidates)
        : Backend::MediaEntrySourceWithCandidateSnapshot(std::move(candidates))
    {
    }

    kiriview::MediaEntrySourceImageDataResult loadImageData(const QUrl&) override
    {
        return kiriview::MediaEntrySourceImageData {};
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
    void storedZipVideoEntryReturnsPlaybackDevice();
    void deflatedZipVideoEntryDoesNotReturnPlaybackDevice();
    void plainTarVideoEntryReturnsPlaybackDevice();
    void unsupportedCollectionVideosDoNotReturnPlaybackDevices();
    void readingUrlOutsideArchiveReturnsNotFound();
    void sourceErrorsPreserveBackendOperationAndIdentity();
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

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    const kiriview::MediaEntrySourceCandidatesResult result
        = kiriview::loadMediaEntrySourceCandidates(*archiveCollection);
    const kiriview::MediaEntrySourceCandidates* success = mediaEntrySourceCandidates(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->candidates.size(), std::size_t(3));
    QCOMPARE(success->candidates.at(0).name, QStringLiteral("chapter/01.png"));
    QCOMPARE(success->candidates.at(0).url,
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    QCOMPARE(success->candidates.at(0).kind, kiriview::ImageDocumentPageKind::Image);
    QCOMPARE(success->candidates.at(1).name, QStringLiteral("chapter/02.jpg"));
    QCOMPARE(success->candidates.at(1).kind, kiriview::ImageDocumentPageKind::Image);
    QCOMPARE(success->candidates.at(2).name, QStringLiteral("chapter/03.mp4"));
    QCOMPARE(success->candidates.at(2).kind, kiriview::ImageDocumentPageKind::Video);
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

    const std::optional<kiriview::OpenedCollectionScopeLocation> directoryCollection
        = kiriview::openedCollectionScopeLocationForDirectlyOpenedLocalUrl(localUrl(dir.path()));
    QVERIFY(directoryCollection.has_value());
    QCOMPARE(directoryCollection->kind(), kiriview::OpenedCollectionScopeKind::Directory);
    const kiriview::MediaEntrySourceCandidatesResult result
        = kiriview::loadMediaEntrySourceCandidates(*directoryCollection);
    const kiriview::MediaEntrySourceCandidates* success = mediaEntrySourceCandidates(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->candidates.size(), std::size_t(3));
    QCOMPARE(success->candidates.at(0).name, QStringLiteral("chapter/01.png"));
    QCOMPARE(success->candidates.at(0).url,
        archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    QCOMPARE(success->candidates.at(0).kind, kiriview::ImageDocumentPageKind::Image);
    QCOMPARE(success->candidates.at(1).name, QStringLiteral("chapter/02.jpg"));
    QCOMPARE(success->candidates.at(1).kind, kiriview::ImageDocumentPageKind::Image);
    QCOMPARE(success->candidates.at(2).name, QStringLiteral("chapter/03.m4v"));
    QCOMPARE(success->candidates.at(2).kind, kiriview::ImageDocumentPageKind::Video);
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

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    const kiriview::MediaEntrySourceCandidatesResult result
        = kiriview::loadMediaEntrySourceCandidates(*archiveCollection);
    const kiriview::MediaEntrySourceCandidates* success = mediaEntrySourceCandidates(result);

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

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    QCOMPARE(archiveCollection->rootUrl().scheme(), QStringLiteral("rar"));
    const kiriview::MediaEntrySourceCandidatesResult result
        = kiriview::loadMediaEntrySourceCandidates(*archiveCollection);
    const kiriview::MediaEntrySourceCandidates* success = mediaEntrySourceCandidates(result);

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

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    const kiriview::MediaEntrySourceImageDataResult result
        = kiriview::loadMediaEntrySourceImageData(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("page.png")));
    const kiriview::MediaEntrySourceImageData* success = mediaEntrySourceImageData(result);

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

    const std::optional<kiriview::OpenedCollectionScopeLocation> directoryCollection
        = kiriview::openedCollectionScopeLocationForDirectlyOpenedLocalUrl(localUrl(dir.path()));
    QVERIFY(directoryCollection.has_value());
    const kiriview::MediaEntrySourceImageDataResult result
        = kiriview::loadMediaEntrySourceImageData(*directoryCollection,
            archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("pages/page.png")));
    const kiriview::MediaEntrySourceImageData* success = mediaEntrySourceImageData(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->data, expected);
}

void TestMediaEntrySourceBackend::readingRarEntryReturnsOriginalBytes()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.rar"));
    writeRarArchive(archivePath);

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    QCOMPARE(archiveCollection->kind(), kiriview::OpenedCollectionScopeKind::GeneralArchive);
    const kiriview::MediaEntrySourceImageDataResult result
        = kiriview::loadMediaEntrySourceImageData(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/02.jpg")));
    const kiriview::MediaEntrySourceImageData* success = mediaEntrySourceImageData(result);

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

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    kiriview::MediaEntrySourceOpenResult opened
        = kiriview::openMediaEntrySource(*archiveCollection);
    auto* source = std::get_if<kiriview::MediaEntrySourcePtr>(&opened);
    QVERIFY(source != nullptr);
    QVERIFY(*source != nullptr);

    const kiriview::MediaEntrySourceCandidatesResult standaloneCandidatesResult
        = kiriview::loadMediaEntrySourceCandidates(*archiveCollection);
    const kiriview::MediaEntrySourceCandidatesResult sourceCandidatesResult
        = (*source)->loadImageDocumentPageCandidates();
    const kiriview::MediaEntrySourceCandidates* standaloneCandidates
        = mediaEntrySourceCandidates(standaloneCandidatesResult);
    const kiriview::MediaEntrySourceCandidates* sourceCandidates
        = mediaEntrySourceCandidates(sourceCandidatesResult);
    QVERIFY(standaloneCandidates != nullptr);
    QVERIFY(sourceCandidates != nullptr);
    QCOMPARE(standaloneCandidates->candidates.size(), sourceCandidates->candidates.size());
    QCOMPARE(standaloneCandidates->candidates.at(0).name, sourceCandidates->candidates.at(0).name);
    QCOMPARE(standaloneCandidates->candidates.at(1).name, sourceCandidates->candidates.at(1).name);

    const QUrl imageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/01.png"));
    const kiriview::MediaEntrySourceImageDataResult standaloneDataResult
        = kiriview::loadMediaEntrySourceImageData(*archiveCollection, imageUrl);
    const kiriview::MediaEntrySourceImageDataResult sourceDataResult
        = (*source)->loadImageData(imageUrl);
    const kiriview::MediaEntrySourceImageData* standaloneData
        = mediaEntrySourceImageData(standaloneDataResult);
    const kiriview::MediaEntrySourceImageData* sourceData
        = mediaEntrySourceImageData(sourceDataResult);
    QVERIFY(standaloneData != nullptr);
    QVERIFY(sourceData != nullptr);
    QCOMPARE(standaloneData->data, sourceData->data);
}

void TestMediaEntrySourceBackend::candidateSnapshotSourcesOwnSortedDefensiveListing()
{
    const QUrl archiveRootUrl(QStringLiteral("zip:///books/book.cbz/"));
    CandidateSnapshotSource source({
        kiriview::ImageDocumentPageCandidate {
            archivePageUrl(archiveRootUrl, QStringLiteral("pages/02.jpg")),
            QStringLiteral("pages/02.jpg"),
        },
        kiriview::ImageDocumentPageCandidate {
            archivePageUrl(archiveRootUrl, QStringLiteral("pages/01.png")),
            QStringLiteral("pages/01.png"),
        },
    });

    kiriview::MediaEntrySourceCandidatesResult firstResult
        = source.loadImageDocumentPageCandidates();
    auto* first = std::get_if<kiriview::MediaEntrySourceCandidates>(&firstResult);
    QVERIFY(first != nullptr);
    QCOMPARE(first->candidates.size(), std::size_t(2));
    QCOMPARE(first->candidates.at(0).name, QStringLiteral("pages/01.png"));
    QCOMPARE(first->candidates.at(1).name, QStringLiteral("pages/02.jpg"));

    first->candidates.clear();

    const kiriview::MediaEntrySourceCandidatesResult secondResult
        = source.loadImageDocumentPageCandidates();
    const kiriview::MediaEntrySourceCandidates* second = mediaEntrySourceCandidates(secondResult);
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

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    kiriview::MediaEntrySourceOpenResult opened
        = kiriview::openMediaEntrySource(*archiveCollection);
    auto* source = std::get_if<kiriview::MediaEntrySourcePtr>(&opened);
    QVERIFY(source != nullptr);
    QVERIFY(*source != nullptr);

    const kiriview::MediaEntrySourceCandidatesResult candidatesResult
        = (*source)->loadImageDocumentPageCandidates();
    const kiriview::MediaEntrySourceCandidates* candidates
        = mediaEntrySourceCandidates(candidatesResult);
    QVERIFY(candidates != nullptr);
    QCOMPARE(candidates->candidates.size(), std::size_t(2));
    QCOMPARE(candidates->candidates.at(0).name, QStringLiteral("pages/01.png"));

    const kiriview::MediaEntrySourceImageDataResult secondDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/02.jpg")));
    const kiriview::MediaEntrySourceImageData* secondData
        = mediaEntrySourceImageData(secondDataResult);
    QVERIFY(secondData != nullptr);
    QCOMPARE(secondData->data, QByteArrayLiteral("two"));

    const kiriview::MediaEntrySourceImageDataResult firstDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/01.png")));
    const kiriview::MediaEntrySourceImageData* firstData
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

    const std::optional<kiriview::OpenedCollectionScopeLocation> directoryCollection
        = kiriview::openedCollectionScopeLocationForDirectlyOpenedLocalUrl(localUrl(dir.path()));
    QVERIFY(directoryCollection.has_value());
    kiriview::MediaEntrySourceOpenResult opened
        = kiriview::openMediaEntrySource(*directoryCollection);
    auto* source = std::get_if<kiriview::MediaEntrySourcePtr>(&opened);
    QVERIFY(source != nullptr);
    QVERIFY(*source != nullptr);

    const kiriview::MediaEntrySourceCandidatesResult candidatesResult
        = (*source)->loadImageDocumentPageCandidates();
    const kiriview::MediaEntrySourceCandidates* candidates
        = mediaEntrySourceCandidates(candidatesResult);
    QVERIFY(candidates != nullptr);
    QCOMPARE(candidates->candidates.size(), std::size_t(2));
    QCOMPARE(candidates->candidates.at(0).name, QStringLiteral("pages/01.png"));

    const kiriview::MediaEntrySourceImageDataResult secondDataResult = (*source)->loadImageData(
        archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("pages/02.jpg")));
    const kiriview::MediaEntrySourceImageData* secondData
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

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    kiriview::MediaEntrySourceOpenResult opened
        = kiriview::openMediaEntrySource(*archiveCollection);
    auto* source = std::get_if<kiriview::MediaEntrySourcePtr>(&opened);
    QVERIFY(source != nullptr);
    QVERIFY(*source != nullptr);

    const kiriview::MediaEntrySourceCandidatesResult candidatesResult
        = (*source)->loadImageDocumentPageCandidates();
    const kiriview::MediaEntrySourceCandidates* candidates
        = mediaEntrySourceCandidates(candidatesResult);
    QVERIFY(candidates != nullptr);
    QCOMPARE(candidates->candidates.size(), std::size_t(2));
    QCOMPARE(candidates->candidates.at(0).name, QStringLiteral("chapter/01.png"));
    QCOMPARE(candidates->candidates.at(1).name, QStringLiteral("chapter/02.jpg"));

    // The fixture stores 02.jpg before 01.png, so this read order exercises replay.
    const kiriview::MediaEntrySourceImageDataResult firstDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    const kiriview::MediaEntrySourceImageData* firstData
        = mediaEntrySourceImageData(firstDataResult);
    QVERIFY(firstData != nullptr);
    QCOMPARE(firstData->data, QByteArrayLiteral("one"));

    const kiriview::MediaEntrySourceImageDataResult secondDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/02.jpg")));
    const kiriview::MediaEntrySourceImageData* secondData
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

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    kiriview::MediaEntrySourceOpenResult opened
        = kiriview::openMediaEntrySource(*archiveCollection);
    auto* source = std::get_if<kiriview::MediaEntrySourcePtr>(&opened);
    QVERIFY(source != nullptr);
    QVERIFY(*source != nullptr);

    const kiriview::MediaEntrySourceCandidatesResult candidatesResult
        = (*source)->loadImageDocumentPageCandidates();
    const kiriview::MediaEntrySourceCandidates* candidates
        = mediaEntrySourceCandidates(candidatesResult);
    QVERIFY(candidates != nullptr);
    QCOMPARE(candidates->candidates.size(), std::size_t(2));

    QVERIFY(QFile::remove(archivePath));

    const kiriview::MediaEntrySourceImageDataResult firstDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    const kiriview::MediaEntrySourceImageData* firstData
        = mediaEntrySourceImageData(firstDataResult);
    QVERIFY(firstData != nullptr);
    QCOMPARE(firstData->data, QByteArrayLiteral("one"));

    const kiriview::MediaEntrySourceImageDataResult secondDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/02.jpg")));
    const kiriview::MediaEntrySourceImageData* secondData
        = mediaEntrySourceImageData(secondDataResult);
    QVERIFY(secondData != nullptr);
    QCOMPARE(secondData->data, QByteArrayLiteral("two"));

    const kiriview::MediaEntrySourceImageDataResult standaloneResult
        = kiriview::loadMediaEntrySourceImageData(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    const kiriview::MediaEntrySourceError* standaloneError
        = mediaEntrySourceDataError(standaloneResult);
    QVERIFY(standaloneError != nullptr);
    QVERIFY(!standaloneError->errorString.isEmpty());
}

void TestMediaEntrySourceBackend::openedCollectionThumbnailPolicyAllowsZipBackedImagesOnly()
{
    using Kind = kiriview::OpenedCollectionThumbnailSourcePlanKind;

    const kiriview::OpenedCollectionScopeLocation cbzScope
        = kiriview::OpenedCollectionScopeLocation::fromUrls(
            localUrl(QStringLiteral("/books/book.cbz")),
            QUrl(QStringLiteral("zip:///books/book.cbz/")),
            kiriview::OpenedCollectionScopeKind::ComicBookArchive);
    const QUrl cbzPage = archivePageUrl(cbzScope.rootUrl(), QStringLiteral("pages/001.png"));
    QCOMPARE(kiriview::openedCollectionThumbnailSourcePlan(
                 cbzScope, cbzPage, kiriview::ImageDocumentPageKind::Image)
                 .kind,
        Kind::CacheableOpenedCollectionEntry);
    QCOMPARE(kiriview::openedCollectionThumbnailSourcePlan(
                 cbzScope, cbzPage, kiriview::ImageDocumentPageKind::Image)
                 .openedCollectionScope,
        cbzScope);
    QCOMPARE(kiriview::openedCollectionThumbnailSourcePlan(
                 cbzScope, cbzPage, kiriview::ImageDocumentPageKind::Video)
                 .kind,
        Kind::Unsupported);

    const kiriview::OpenedCollectionScopeLocation cb7Scope
        = kiriview::OpenedCollectionScopeLocation::fromUrls(
            localUrl(QStringLiteral("/books/book.cb7")),
            QUrl(QStringLiteral("sevenz:///books/book.cb7/")),
            kiriview::OpenedCollectionScopeKind::ComicBookArchive);
    QCOMPARE(kiriview::openedCollectionThumbnailSourcePlan(cb7Scope,
                 archivePageUrl(cb7Scope.rootUrl(), QStringLiteral("pages/001.jpg")),
                 kiriview::ImageDocumentPageKind::Image)
                 .kind,
        Kind::Unsupported);

    const kiriview::OpenedCollectionScopeLocation cbtScope
        = kiriview::OpenedCollectionScopeLocation::fromUrls(
            localUrl(QStringLiteral("/books/book.cbt")),
            QUrl(QStringLiteral("tar:///books/book.cbt/")),
            kiriview::OpenedCollectionScopeKind::ComicBookArchive);
    QCOMPARE(kiriview::openedCollectionThumbnailSourcePlan(cbtScope,
                 archivePageUrl(cbtScope.rootUrl(), QStringLiteral("pages/001.png")),
                 kiriview::ImageDocumentPageKind::Image)
                 .kind,
        Kind::Unsupported);

    const kiriview::OpenedCollectionScopeLocation genericZipScope
        = kiriview::OpenedCollectionScopeLocation::fromUrls(
            localUrl(QStringLiteral("/books/book.zip")),
            QUrl(QStringLiteral("zip:///books/book.zip/")),
            kiriview::OpenedCollectionScopeKind::GeneralArchive);
    QCOMPARE(kiriview::openedCollectionThumbnailSourcePlan(genericZipScope,
                 archivePageUrl(genericZipScope.rootUrl(), QStringLiteral("pages/001.png")),
                 kiriview::ImageDocumentPageKind::Image)
                 .kind,
        Kind::CacheableOpenedCollectionEntry);

    const kiriview::OpenedCollectionScopeLocation directoryScope
        = kiriview::OpenedCollectionScopeLocation::fromUrls(localUrl(QStringLiteral("/books/dir")),
            localUrl(QStringLiteral("/books/dir/")),
            kiriview::OpenedCollectionScopeKind::Directory);
    QCOMPARE(kiriview::openedCollectionThumbnailSourcePlan(directoryScope,
                 archivePageUrl(directoryScope.rootUrl(), QStringLiteral("pages/001.png")),
                 kiriview::ImageDocumentPageKind::Image)
                 .kind,
        Kind::Unsupported);

    QCOMPARE(kiriview::openedCollectionThumbnailSourcePlan(cbzScope,
                 archivePageUrl(QUrl(QStringLiteral("zip:///books/other.cbz/")),
                     QStringLiteral("pages/001.png")),
                 kiriview::ImageDocumentPageKind::Image)
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

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    const kiriview::MediaEntrySourceThumbnailMetadataResult result
        = kiriview::loadMediaEntrySourceThumbnailMetadata(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/01.png")));
    const kiriview::MediaEntrySourceThumbnailMetadata* metadata
        = mediaEntrySourceThumbnailMetadata(result);

    QVERIFY(metadata != nullptr);
    QCOMPARE(metadata->checksum.algorithm, kiriview::MediaEntryContentChecksumAlgorithm::Crc32);
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

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    const kiriview::MediaEntrySourceThumbnailMetadataResult result
        = kiriview::loadMediaEntrySourceThumbnailMetadata(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/01.png")));
    const kiriview::MediaEntrySourceThumbnailMetadata* metadata
        = mediaEntrySourceThumbnailMetadata(result);

    QVERIFY(metadata != nullptr);
    QCOMPARE(metadata->checksum.algorithm, kiriview::MediaEntryContentChecksumAlgorithm::Crc32);
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

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    const kiriview::MediaEntrySourceThumbnailMetadataResult result
        = kiriview::loadMediaEntrySourceThumbnailMetadata(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/01.png")));
    QVERIFY(std::get_if<kiriview::MediaEntrySourceError>(&result) != nullptr);
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

    const std::optional<kiriview::OpenedCollectionScopeLocation> cbtCollection
        = archiveCollectionForPath(cbtPath);
    const std::optional<kiriview::OpenedCollectionScopeLocation> cbrCollection
        = archiveCollectionForPath(cbrPath);
    QVERIFY(cbtCollection.has_value());
    QVERIFY(cbrCollection.has_value());

    const kiriview::MediaEntrySourceThumbnailMetadataResult cbtResult
        = kiriview::loadMediaEntrySourceThumbnailMetadata(*cbtCollection,
            archivePageUrl(cbtCollection->rootUrl(), QStringLiteral("pages/01.png")));
    QVERIFY(std::get_if<kiriview::MediaEntrySourceError>(&cbtResult) != nullptr);

    const kiriview::MediaEntrySourceThumbnailMetadataResult cbrResult
        = kiriview::loadMediaEntrySourceThumbnailMetadata(*cbrCollection,
            archivePageUrl(cbrCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    QVERIFY(std::get_if<kiriview::MediaEntrySourceError>(&cbrResult) != nullptr);

    const std::optional<kiriview::OpenedCollectionScopeLocation> directoryCollection
        = kiriview::openedCollectionScopeLocationForDirectlyOpenedLocalUrl(localUrl(dir.path()));
    QVERIFY(directoryCollection.has_value());
    const kiriview::MediaEntrySourceThumbnailMetadataResult directoryResult
        = kiriview::loadMediaEntrySourceThumbnailMetadata(*directoryCollection,
            archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("pages/01.png")));
    QVERIFY(std::get_if<kiriview::MediaEntrySourceError>(&directoryResult) != nullptr);

    const QString cbzPath = dir.filePath(QStringLiteral("book.cbz"));
    writeZipArchive(cbzPath,
        {
            { QStringLiteral("pages/clip.mp4"), QByteArrayLiteral("video") },
        });
    const std::optional<kiriview::OpenedCollectionScopeLocation> cbzCollection
        = archiveCollectionForPath(cbzPath);
    QVERIFY(cbzCollection.has_value());
    const kiriview::MediaEntrySourceThumbnailMetadataResult videoResult
        = kiriview::loadMediaEntrySourceThumbnailMetadata(*cbzCollection,
            archivePageUrl(cbzCollection->rootUrl(), QStringLiteral("pages/clip.mp4")));
    QVERIFY(std::get_if<kiriview::MediaEntrySourceError>(&videoResult) != nullptr);
}

void TestMediaEntrySourceBackend::storedZipVideoEntryReturnsPlaybackDevice()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.cbz"));
    const QByteArray expected = QByteArrayLiteral("video-bytes");
    writeZipArchiveWithCompression(archivePath,
        {
            { QStringLiteral("pages/clip.mp4"), expected },
        },
        KZip::NoCompression);

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    kiriview::MediaEntrySourceVideoPlaybackDeviceResult result
        = kiriview::loadMediaEntrySourceVideoPlaybackDevice(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/clip.mp4")));
    kiriview::MediaEntrySourceVideoPlaybackDevice* device
        = mediaEntrySourceVideoPlaybackDevice(result);

    QVERIFY(device != nullptr);
    QVERIFY(device->sourceOwner != nullptr);
    QVERIFY(device->device != nullptr);
    QCOMPARE(device->device->readAll(), expected);
}

void TestMediaEntrySourceBackend::deflatedZipVideoEntryDoesNotReturnPlaybackDevice()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.zip"));
    writeZipArchiveWithCompression(archivePath,
        {
            { QStringLiteral("pages/clip.mp4"), QByteArrayLiteral("video-bytes") },
        },
        KZip::DeflateCompression);

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    kiriview::MediaEntrySourceVideoPlaybackDeviceResult result
        = kiriview::loadMediaEntrySourceVideoPlaybackDevice(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/clip.mp4")));

    const kiriview::MediaEntrySourceError* error
        = std::get_if<kiriview::MediaEntrySourceError>(&result);
    QVERIFY(error != nullptr);
    QCOMPARE(error->backend, kiriview::MediaEntrySourceBackendKind::KArchive);
    QCOMPARE(error->operation, kiriview::MediaEntrySourceOperation::OpenVideoPlaybackDevice);
    QCOMPARE(error->entryPath, QStringLiteral("pages/clip.mp4"));
}

void TestMediaEntrySourceBackend::plainTarVideoEntryReturnsPlaybackDevice()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString archivePath = dir.filePath(QStringLiteral("book.tar"));
    const QByteArray expected = QByteArrayLiteral("video-bytes");
    writeTarArchive(archivePath,
        {
            { QStringLiteral("pages/clip.mov"), expected },
        });

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    kiriview::MediaEntrySourceVideoPlaybackDeviceResult result
        = kiriview::loadMediaEntrySourceVideoPlaybackDevice(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/clip.mov")));
    kiriview::MediaEntrySourceVideoPlaybackDevice* device
        = mediaEntrySourceVideoPlaybackDevice(result);

    QVERIFY(device != nullptr);
    QVERIFY(device->sourceOwner != nullptr);
    QVERIFY(device->device != nullptr);
    QCOMPARE(device->device->readAll(), expected);
}

void TestMediaEntrySourceBackend::unsupportedCollectionVideosDoNotReturnPlaybackDevices()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QDir root(dir.path());
    QVERIFY(root.mkpath(QStringLiteral("pages")));
    writeFile(dir.filePath(QStringLiteral("pages/clip.mp4")), QByteArrayLiteral("video-bytes"));

    const std::optional<kiriview::OpenedCollectionScopeLocation> directoryCollection
        = kiriview::openedCollectionScopeLocationForDirectlyOpenedLocalUrl(localUrl(dir.path()));
    QVERIFY(directoryCollection.has_value());
    kiriview::MediaEntrySourceVideoPlaybackDeviceResult directoryResult
        = kiriview::loadMediaEntrySourceVideoPlaybackDevice(*directoryCollection,
            archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("pages/clip.mp4")));
    const kiriview::MediaEntrySourceError* directoryError
        = std::get_if<kiriview::MediaEntrySourceError>(&directoryResult);
    QVERIFY(directoryError != nullptr);
    QCOMPARE(directoryError->backend, kiriview::MediaEntrySourceBackendKind::Directory);
    QCOMPARE(
        directoryError->operation, kiriview::MediaEntrySourceOperation::OpenVideoPlaybackDevice);

    const QString archivePath = dir.filePath(QStringLiteral("book.cb7"));
    write7zArchive(archivePath,
        {
            { QStringLiteral("pages/clip.m4v"), QByteArrayLiteral("video-bytes") },
        });
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    kiriview::MediaEntrySourceVideoPlaybackDeviceResult sevenZipResult
        = kiriview::loadMediaEntrySourceVideoPlaybackDevice(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/clip.m4v")));
    const kiriview::MediaEntrySourceError* sevenZipError
        = std::get_if<kiriview::MediaEntrySourceError>(&sevenZipResult);
    QVERIFY(sevenZipError != nullptr);
    QCOMPARE(sevenZipError->backend, kiriview::MediaEntrySourceBackendKind::KArchive);
    QCOMPARE(
        sevenZipError->operation, kiriview::MediaEntrySourceOperation::OpenVideoPlaybackDevice);
    QCOMPARE(sevenZipError->entryPath, QStringLiteral("pages/clip.m4v"));
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

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    const kiriview::MediaEntrySourceImageDataResult result
        = kiriview::loadMediaEntrySourceImageData(
            *archiveCollection, localUrl(QStringLiteral("/outside/page.png")));
    const kiriview::MediaEntrySourceError* error = mediaEntrySourceDataError(result);

    QVERIFY(error != nullptr);
    QVERIFY(!error->errorString.isEmpty());
}

void TestMediaEntrySourceBackend::sourceErrorsPreserveBackendOperationAndIdentity()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const kiriview::OpenedCollectionScopeLocation missingArchive
        = kiriview::OpenedCollectionScopeLocation::fromUrls(
            localUrl(dir.filePath(QStringLiteral("missing.cbz"))),
            QUrl(QStringLiteral("zip:///missing.cbz/")),
            kiriview::OpenedCollectionScopeKind::ComicBookArchive);
    const kiriview::MediaEntrySourceCandidatesResult missingArchiveResult
        = kiriview::loadMediaEntrySourceCandidates(missingArchive);
    const kiriview::MediaEntrySourceError* missingArchiveError
        = mediaEntrySourceError(missingArchiveResult);

    QVERIFY(missingArchiveError != nullptr);
    QCOMPARE(missingArchiveError->backend, kiriview::MediaEntrySourceBackendKind::KArchive);
    QCOMPARE(missingArchiveError->operation, kiriview::MediaEntrySourceOperation::OpenCollection);
    QCOMPARE(missingArchiveError->collectionUrl, missingArchive.fileUrl());
    QVERIFY(missingArchiveError->entryPath.isEmpty());
    QVERIFY(!missingArchiveError->errorString.isEmpty());
    QVERIFY(!missingArchiveError->diagnosticDetail.isEmpty());

    QDir root(dir.path());
    QVERIFY(root.mkpath(QStringLiteral("directory")));
    const std::optional<kiriview::OpenedCollectionScopeLocation> directoryCollection
        = kiriview::openedCollectionScopeLocationForDirectlyOpenedLocalUrl(
            localUrl(dir.filePath(QStringLiteral("directory"))));
    QVERIFY(directoryCollection.has_value());
    const QUrl missingEntryUrl
        = archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("missing.png"));
    const kiriview::MediaEntrySourceImageDataResult missingEntryResult
        = kiriview::loadMediaEntrySourceImageData(*directoryCollection, missingEntryUrl);
    const kiriview::MediaEntrySourceError* missingEntryError
        = mediaEntrySourceDataError(missingEntryResult);

    QVERIFY(missingEntryError != nullptr);
    QCOMPARE(missingEntryError->backend, kiriview::MediaEntrySourceBackendKind::Directory);
    QCOMPARE(missingEntryError->operation, kiriview::MediaEntrySourceOperation::ReadImageData);
    QCOMPARE(missingEntryError->collectionUrl, directoryCollection->fileUrl());
    QCOMPARE(missingEntryError->entryPath, QStringLiteral("missing.png"));
    QVERIFY(!missingEntryError->errorString.isEmpty());
    QVERIFY(!missingEntryError->diagnosticDetail.isEmpty());

    const QString invalidRarPath = dir.filePath(QStringLiteral("invalid.cbr"));
    writeFile(invalidRarPath, QByteArrayLiteral("not an archive"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> invalidRarCollection
        = archiveCollectionForPath(invalidRarPath);
    QVERIFY(invalidRarCollection.has_value());
    const kiriview::MediaEntrySourceCandidatesResult invalidRarResult
        = kiriview::loadMediaEntrySourceCandidates(*invalidRarCollection);
    const kiriview::MediaEntrySourceError* invalidRarError
        = mediaEntrySourceError(invalidRarResult);

    QVERIFY(invalidRarError != nullptr);
    QCOMPARE(invalidRarError->backend, kiriview::MediaEntrySourceBackendKind::LibArchive);
    QCOMPARE(invalidRarError->operation, kiriview::MediaEntrySourceOperation::OpenCollection);
    QCOMPARE(invalidRarError->collectionUrl, invalidRarCollection->fileUrl());
    QVERIFY(invalidRarError->entryPath.isEmpty());
    QVERIFY(!invalidRarError->errorString.isEmpty());
    QVERIFY(!invalidRarError->diagnosticDetail.isEmpty());
}

void TestMediaEntrySourceBackend::missingEmptyAndInvalidArchivesReportExpectedResults()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const kiriview::OpenedCollectionScopeLocation missingArchive
        = kiriview::OpenedCollectionScopeLocation::fromUrls(
            localUrl(dir.filePath(QStringLiteral("missing.cbz"))),
            QUrl(QStringLiteral("zip:///missing.cbz/")),
            kiriview::OpenedCollectionScopeKind::ComicBookArchive);
    const kiriview::MediaEntrySourceCandidatesResult missingResult
        = kiriview::loadMediaEntrySourceCandidates(missingArchive);
    const kiriview::MediaEntrySourceError* missingError = mediaEntrySourceError(missingResult);
    QVERIFY(missingError != nullptr);
    QVERIFY(!missingError->errorString.isEmpty());

    const QString emptyArchivePath = dir.filePath(QStringLiteral("empty.cbz"));
    writeZipArchive(emptyArchivePath, {});
    const std::optional<kiriview::OpenedCollectionScopeLocation> emptyArchiveCollection
        = archiveCollectionForPath(emptyArchivePath);
    QVERIFY(emptyArchiveCollection.has_value());
    const kiriview::MediaEntrySourceCandidatesResult emptyResult
        = kiriview::loadMediaEntrySourceCandidates(*emptyArchiveCollection);
    const kiriview::MediaEntrySourceCandidates* emptySuccess
        = mediaEntrySourceCandidates(emptyResult);
    QVERIFY(emptySuccess != nullptr);
    QVERIFY(emptySuccess->candidates.empty());

    const QString invalidArchivePath = dir.filePath(QStringLiteral("invalid.cbz"));
    QFile invalidArchive(invalidArchivePath);
    QVERIFY(invalidArchive.open(QIODevice::WriteOnly));
    QCOMPARE(invalidArchive.write(QByteArrayLiteral("not an archive")),
        qsizetype(QByteArrayLiteral("not an archive").size()));
    invalidArchive.close();

    const std::optional<kiriview::OpenedCollectionScopeLocation> invalidArchiveCollection
        = archiveCollectionForPath(invalidArchivePath);
    QVERIFY(invalidArchiveCollection.has_value());
    const kiriview::MediaEntrySourceCandidatesResult invalidResult
        = kiriview::loadMediaEntrySourceCandidates(*invalidArchiveCollection);
    const kiriview::MediaEntrySourceError* invalidError = mediaEntrySourceError(invalidResult);
    QVERIFY(invalidError != nullptr);
    QVERIFY(!invalidError->errorString.isEmpty());
}

QTEST_GUILESS_MAIN(TestMediaEntrySourceBackend)

#include "test_mediaentrysourcebackend.moc"
