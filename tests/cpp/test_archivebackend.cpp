// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archive/archivebackend.h"

#include "archive/archivebackend_p.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"

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
namespace Backend = KiriView::ArchiveBackendDetail;
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

class CandidateSnapshotSource final : public Backend::MediaEntrySourceWithCandidateSnapshot
{
public:
    explicit CandidateSnapshotSource(std::vector<KiriView::ImageNavigationCandidate> candidates)
        : Backend::MediaEntrySourceWithCandidateSnapshot(std::move(candidates))
    {
    }

    KiriView::ArchiveImageDataResult loadImageData(const QUrl &) override
    {
        return KiriView::ArchiveImageData {};
    }
};
}

class TestArchiveBackend : public QObject
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
    void directoryMediaEntrySourceListsAndReadsEntries();
    void libArchiveMediaEntrySourceScansOnceAndServesRandomReads();
    void libArchiveMediaEntrySourceReadsFromHeldFileDescriptorAfterPathRemoval();
    void readingUrlOutsideArchiveReturnsNotFound();
    void missingEmptyAndInvalidArchivesReportExpectedResults();
};

void TestArchiveBackend::zipListingIncludesNestedSupportedMedia()
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
    const KiriView::ArchiveImageCandidatesResult result
        = KiriView::loadMediaEntrySourceCandidates(*archiveCollection);
    const KiriView::ArchiveImageCandidates *success = archiveImageCandidates(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->candidates.size(), std::size_t(3));
    QCOMPARE(success->candidates.at(0).name, QStringLiteral("chapter/01.png"));
    QCOMPARE(success->candidates.at(0).url,
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    QCOMPARE(success->candidates.at(0).kind, KiriView::ImageNavigationCandidateKind::Image);
    QCOMPARE(success->candidates.at(1).name, QStringLiteral("chapter/02.jpg"));
    QCOMPARE(success->candidates.at(1).kind, KiriView::ImageNavigationCandidateKind::Image);
    QCOMPARE(success->candidates.at(2).name, QStringLiteral("chapter/03.mp4"));
    QCOMPARE(success->candidates.at(2).kind, KiriView::ImageNavigationCandidateKind::Video);
}

void TestArchiveBackend::directoryListingIncludesNestedSupportedMedia()
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
    const KiriView::ArchiveImageCandidatesResult result
        = KiriView::loadMediaEntrySourceCandidates(*directoryCollection);
    const KiriView::ArchiveImageCandidates *success = archiveImageCandidates(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->candidates.size(), std::size_t(3));
    QCOMPARE(success->candidates.at(0).name, QStringLiteral("chapter/01.png"));
    QCOMPARE(success->candidates.at(0).url,
        archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    QCOMPARE(success->candidates.at(0).kind, KiriView::ImageNavigationCandidateKind::Image);
    QCOMPARE(success->candidates.at(1).name, QStringLiteral("chapter/02.jpg"));
    QCOMPARE(success->candidates.at(1).kind, KiriView::ImageNavigationCandidateKind::Image);
    QCOMPARE(success->candidates.at(2).name, QStringLiteral("chapter/03.m4v"));
    QCOMPARE(success->candidates.at(2).kind, KiriView::ImageNavigationCandidateKind::Video);
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

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    const KiriView::ArchiveImageCandidatesResult result
        = KiriView::loadMediaEntrySourceCandidates(*archiveCollection);
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

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    QCOMPARE(archiveCollection->rootUrl().scheme(), QStringLiteral("rar"));
    const KiriView::ArchiveImageCandidatesResult result
        = KiriView::loadMediaEntrySourceCandidates(*archiveCollection);
    const KiriView::ArchiveImageCandidates *success = archiveImageCandidates(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->candidates.size(), std::size_t(2));
    QCOMPARE(success->candidates.at(0).name, QStringLiteral("chapter/01.png"));
    QCOMPARE(success->candidates.at(0).url,
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/01.png")));
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

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    const KiriView::ArchiveImageDataResult result
        = KiriView::loadMediaEntrySourceImageData(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("page.png")));
    const KiriView::ArchiveImageData *success = archiveImageData(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->data, expected);
}

void TestArchiveBackend::readingDirectoryEntryReturnsOriginalBytes()
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
    const KiriView::ArchiveImageDataResult result
        = KiriView::loadMediaEntrySourceImageData(*directoryCollection,
            archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("pages/page.png")));
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

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    QCOMPARE(archiveCollection->kind(), KiriView::OpenedCollectionScopeKind::GeneralArchive);
    const KiriView::ArchiveImageDataResult result
        = KiriView::loadMediaEntrySourceImageData(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/02.jpg")));
    const KiriView::ArchiveImageData *success = archiveImageData(result);

    QVERIFY(success != nullptr);
    QCOMPARE(success->data, QByteArrayLiteral("two"));
}

void TestArchiveBackend::standaloneHelpersMatchMediaEntrySourceResults()
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

    const KiriView::ArchiveImageCandidatesResult standaloneCandidatesResult
        = KiriView::loadMediaEntrySourceCandidates(*archiveCollection);
    const KiriView::ArchiveImageCandidatesResult sourceCandidatesResult
        = (*source)->loadImageCandidates();
    const KiriView::ArchiveImageCandidates *standaloneCandidates
        = archiveImageCandidates(standaloneCandidatesResult);
    const KiriView::ArchiveImageCandidates *sourceCandidates
        = archiveImageCandidates(sourceCandidatesResult);
    QVERIFY(standaloneCandidates != nullptr);
    QVERIFY(sourceCandidates != nullptr);
    QCOMPARE(standaloneCandidates->candidates.size(), sourceCandidates->candidates.size());
    QCOMPARE(standaloneCandidates->candidates.at(0).name, sourceCandidates->candidates.at(0).name);
    QCOMPARE(standaloneCandidates->candidates.at(1).name, sourceCandidates->candidates.at(1).name);

    const QUrl imageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/01.png"));
    const KiriView::ArchiveImageDataResult standaloneDataResult
        = KiriView::loadMediaEntrySourceImageData(*archiveCollection, imageUrl);
    const KiriView::ArchiveImageDataResult sourceDataResult = (*source)->loadImageData(imageUrl);
    const KiriView::ArchiveImageData *standaloneData = archiveImageData(standaloneDataResult);
    const KiriView::ArchiveImageData *sourceData = archiveImageData(sourceDataResult);
    QVERIFY(standaloneData != nullptr);
    QVERIFY(sourceData != nullptr);
    QCOMPARE(standaloneData->data, sourceData->data);
}

void TestArchiveBackend::candidateSnapshotSourcesOwnSortedDefensiveListing()
{
    const QUrl archiveRootUrl(QStringLiteral("zip:///books/book.cbz/"));
    CandidateSnapshotSource source({
        KiriView::ImageNavigationCandidate {
            archivePageUrl(archiveRootUrl, QStringLiteral("pages/02.jpg")),
            QStringLiteral("pages/02.jpg"),
        },
        KiriView::ImageNavigationCandidate {
            archivePageUrl(archiveRootUrl, QStringLiteral("pages/01.png")),
            QStringLiteral("pages/01.png"),
        },
    });

    KiriView::ArchiveImageCandidatesResult firstResult = source.loadImageCandidates();
    auto *first = std::get_if<KiriView::ArchiveImageCandidates>(&firstResult);
    QVERIFY(first != nullptr);
    QCOMPARE(first->candidates.size(), std::size_t(2));
    QCOMPARE(first->candidates.at(0).name, QStringLiteral("pages/01.png"));
    QCOMPARE(first->candidates.at(1).name, QStringLiteral("pages/02.jpg"));

    first->candidates.clear();

    const KiriView::ArchiveImageCandidatesResult secondResult = source.loadImageCandidates();
    const KiriView::ArchiveImageCandidates *second = archiveImageCandidates(secondResult);
    QVERIFY(second != nullptr);
    QCOMPARE(second->candidates.size(), std::size_t(2));
    QCOMPARE(second->candidates.at(0).name, QStringLiteral("pages/01.png"));
    QCOMPARE(second->candidates.at(1).name, QStringLiteral("pages/02.jpg"));
}

void TestArchiveBackend::kArchiveMediaEntrySourceListsAndReadsEntries()
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

    const KiriView::ArchiveImageCandidatesResult candidatesResult
        = (*source)->loadImageCandidates();
    const KiriView::ArchiveImageCandidates *candidates = archiveImageCandidates(candidatesResult);
    QVERIFY(candidates != nullptr);
    QCOMPARE(candidates->candidates.size(), std::size_t(2));
    QCOMPARE(candidates->candidates.at(0).name, QStringLiteral("pages/01.png"));

    const KiriView::ArchiveImageDataResult secondDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/02.jpg")));
    const KiriView::ArchiveImageData *secondData = archiveImageData(secondDataResult);
    QVERIFY(secondData != nullptr);
    QCOMPARE(secondData->data, QByteArrayLiteral("two"));

    const KiriView::ArchiveImageDataResult firstDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("pages/01.png")));
    const KiriView::ArchiveImageData *firstData = archiveImageData(firstDataResult);
    QVERIFY(firstData != nullptr);
    QCOMPARE(firstData->data, QByteArrayLiteral("one"));
}

void TestArchiveBackend::directoryMediaEntrySourceListsAndReadsEntries()
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

    const KiriView::ArchiveImageCandidatesResult candidatesResult
        = (*source)->loadImageCandidates();
    const KiriView::ArchiveImageCandidates *candidates = archiveImageCandidates(candidatesResult);
    QVERIFY(candidates != nullptr);
    QCOMPARE(candidates->candidates.size(), std::size_t(2));
    QCOMPARE(candidates->candidates.at(0).name, QStringLiteral("pages/01.png"));

    const KiriView::ArchiveImageDataResult secondDataResult = (*source)->loadImageData(
        archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("pages/02.jpg")));
    const KiriView::ArchiveImageData *secondData = archiveImageData(secondDataResult);
    QVERIFY(secondData != nullptr);
    QCOMPARE(secondData->data, QByteArrayLiteral("two"));
}

void TestArchiveBackend::libArchiveMediaEntrySourceScansOnceAndServesRandomReads()
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

    const KiriView::ArchiveImageCandidatesResult candidatesResult
        = (*source)->loadImageCandidates();
    const KiriView::ArchiveImageCandidates *candidates = archiveImageCandidates(candidatesResult);
    QVERIFY(candidates != nullptr);
    QCOMPARE(candidates->candidates.size(), std::size_t(2));
    QCOMPARE(candidates->candidates.at(0).name, QStringLiteral("chapter/01.png"));
    QCOMPARE(candidates->candidates.at(1).name, QStringLiteral("chapter/02.jpg"));

    // The fixture stores 02.jpg before 01.png, so this read order exercises replay.
    const KiriView::ArchiveImageDataResult firstDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    const KiriView::ArchiveImageData *firstData = archiveImageData(firstDataResult);
    QVERIFY(firstData != nullptr);
    QCOMPARE(firstData->data, QByteArrayLiteral("one"));

    const KiriView::ArchiveImageDataResult secondDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/02.jpg")));
    const KiriView::ArchiveImageData *secondData = archiveImageData(secondDataResult);
    QVERIFY(secondData != nullptr);
    QCOMPARE(secondData->data, QByteArrayLiteral("two"));
}

void TestArchiveBackend::libArchiveMediaEntrySourceReadsFromHeldFileDescriptorAfterPathRemoval()
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

    const KiriView::ArchiveImageCandidatesResult candidatesResult
        = (*source)->loadImageCandidates();
    const KiriView::ArchiveImageCandidates *candidates = archiveImageCandidates(candidatesResult);
    QVERIFY(candidates != nullptr);
    QCOMPARE(candidates->candidates.size(), std::size_t(2));

    QVERIFY(QFile::remove(archivePath));

    const KiriView::ArchiveImageDataResult firstDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    const KiriView::ArchiveImageData *firstData = archiveImageData(firstDataResult);
    QVERIFY(firstData != nullptr);
    QCOMPARE(firstData->data, QByteArrayLiteral("one"));

    const KiriView::ArchiveImageDataResult secondDataResult = (*source)->loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/02.jpg")));
    const KiriView::ArchiveImageData *secondData = archiveImageData(secondDataResult);
    QVERIFY(secondData != nullptr);
    QCOMPARE(secondData->data, QByteArrayLiteral("two"));

    const KiriView::ArchiveImageDataResult standaloneResult
        = KiriView::loadMediaEntrySourceImageData(*archiveCollection,
            archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("chapter/01.png")));
    const KiriView::ArchiveError *standaloneError = archiveDataError(standaloneResult);
    QVERIFY(standaloneError != nullptr);
    QVERIFY(!standaloneError->errorString.isEmpty());
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

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForPath(archivePath);
    QVERIFY(archiveCollection.has_value());
    const KiriView::ArchiveImageDataResult result = KiriView::loadMediaEntrySourceImageData(
        *archiveCollection, localUrl(QStringLiteral("/outside/page.png")));
    const KiriView::ArchiveError *error = archiveDataError(result);

    QVERIFY(error != nullptr);
    QVERIFY(!error->errorString.isEmpty());
}

void TestArchiveBackend::missingEmptyAndInvalidArchivesReportExpectedResults()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const KiriView::OpenedCollectionScopeLocation missingArchive
        = KiriView::OpenedCollectionScopeLocation::fromUrls(
            localUrl(dir.filePath(QStringLiteral("missing.cbz"))),
            QUrl(QStringLiteral("zip:///missing.cbz/")),
            KiriView::OpenedCollectionScopeKind::ComicBookArchive);
    const KiriView::ArchiveImageCandidatesResult missingResult
        = KiriView::loadMediaEntrySourceCandidates(missingArchive);
    const KiriView::ArchiveError *missingError = archiveError(missingResult);
    QVERIFY(missingError != nullptr);
    QVERIFY(!missingError->errorString.isEmpty());

    const QString emptyArchivePath = dir.filePath(QStringLiteral("empty.cbz"));
    writeZipArchive(emptyArchivePath, {});
    const std::optional<KiriView::OpenedCollectionScopeLocation> emptyArchiveCollection
        = archiveCollectionForPath(emptyArchivePath);
    QVERIFY(emptyArchiveCollection.has_value());
    const KiriView::ArchiveImageCandidatesResult emptyResult
        = KiriView::loadMediaEntrySourceCandidates(*emptyArchiveCollection);
    const KiriView::ArchiveImageCandidates *emptySuccess = archiveImageCandidates(emptyResult);
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
    const KiriView::ArchiveImageCandidatesResult invalidResult
        = KiriView::loadMediaEntrySourceCandidates(*invalidArchiveCollection);
    const KiriView::ArchiveError *invalidError = archiveError(invalidResult);
    QVERIFY(invalidError != nullptr);
    QVERIFY(!invalidError->errorString.isEmpty());
}

QTEST_GUILESS_MAIN(TestArchiveBackend)

#include "test_archivebackend.moc"
