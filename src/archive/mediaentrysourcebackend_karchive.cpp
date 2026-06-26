// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaentrysourcebackend_p.h"

#include "archiveformat.h"
#include "openedcollectionthumbnailpolicy.h"

#include <K7Zip>
#include <KArchive>
#include <KArchiveDirectory>
#include <KArchiveFile>
#include <KTar>
#include <KZip>
#include <KZipFileEntry>
#include <QIODevice>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
namespace Backend = kiriview::MediaEntrySourceBackendDetail;

class ScopedKArchive final
{
public:
    ScopedKArchive() = default;

    explicit ScopedKArchive(std::unique_ptr<KArchive> archive)
        : m_archive(std::move(archive))
    {
    }

    ~ScopedKArchive() { close(); }

    ScopedKArchive(const ScopedKArchive&) = delete;
    ScopedKArchive& operator=(const ScopedKArchive&) = delete;
    ScopedKArchive(ScopedKArchive&&) noexcept = default;
    ScopedKArchive& operator=(ScopedKArchive&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }

        close();
        m_archive = std::move(other.m_archive);
        return *this;
    }

    const KArchiveDirectory* directory() const
    {
        return m_archive == nullptr ? nullptr : m_archive->directory();
    }

    explicit operator bool() const { return m_archive != nullptr; }

private:
    void close()
    {
        if (m_archive != nullptr) {
            m_archive->close();
        }
    }

    std::unique_ptr<KArchive> m_archive;
};

struct OpenKArchiveResult
{
    ScopedKArchive archive;
    QString errorString;
};

std::unique_ptr<KArchive> createArchive(
    const kiriview::OpenedCollectionScopeLocation& openedCollectionScope)
{
    const QString filePath = openedCollectionScope.fileUrl().toLocalFile();
    if (filePath.isEmpty()) {
        return nullptr;
    }

    switch (
        kiriview::archiveStorageBackendForRootScheme(openedCollectionScope.rootUrl().scheme())) {
    case kiriview::ArchiveStorageBackend::KZip:
        return std::make_unique<KZip>(filePath);
    case kiriview::ArchiveStorageBackend::KTar:
        return std::make_unique<KTar>(filePath);
    case kiriview::ArchiveStorageBackend::K7Zip:
        return std::make_unique<K7Zip>(filePath);
    case kiriview::ArchiveStorageBackend::LibArchive:
    case kiriview::ArchiveStorageBackend::None:
        return nullptr;
    }

    return nullptr;
}

void appendArchiveDirectoryImageDocumentPageCandidates(
    std::vector<kiriview::ImageDocumentPageCandidate>* candidates,
    const KArchiveDirectory* directory,
    const kiriview::OpenedCollectionScopeLocation& openedCollectionScope, const QString& prefix)
{
    if (directory == nullptr) {
        return;
    }

    const QStringList entries = directory->entries();
    candidates->reserve(candidates->size() + static_cast<std::size_t>(entries.size()));
    for (const QString& entryName : entries) {
        const QString entryPath
            = prefix.isEmpty() ? entryName : prefix + QLatin1Char('/') + entryName;
        const KArchiveEntry* entry = directory->entry(entryName);
        if (entry == nullptr) {
            continue;
        }

        if (entry->isDirectory()) {
            appendArchiveDirectoryImageDocumentPageCandidates(candidates,
                static_cast<const KArchiveDirectory*>(entry), openedCollectionScope, entryPath);
            continue;
        }

        if (!entry->isFile()) {
            continue;
        }

        std::optional<kiriview::ImageDocumentPageCandidate> candidate
            = Backend::openedCollectionImageDocumentPageCandidate(openedCollectionScope, entryPath);
        if (candidate.has_value()) {
            candidates->push_back(std::move(*candidate));
        }
    }
}

std::vector<kiriview::ImageDocumentPageCandidate> archiveDirectoryImageDocumentPageCandidates(
    const KArchiveDirectory* directory,
    const kiriview::OpenedCollectionScopeLocation& openedCollectionScope)
{
    std::vector<kiriview::ImageDocumentPageCandidate> candidates;
    appendArchiveDirectoryImageDocumentPageCandidates(
        &candidates, directory, openedCollectionScope, QString());
    return candidates;
}

OpenKArchiveResult openKArchiveCollection(
    const kiriview::OpenedCollectionScopeLocation& openedCollectionScope)
{
    std::unique_ptr<KArchive> archive = createArchive(openedCollectionScope);
    if (archive == nullptr) {
        return OpenKArchiveResult { {},
            Backend::fallbackMediaEntrySourceOpenError(openedCollectionScope) };
    }

    if (!archive->open(QIODevice::ReadOnly)) {
        const QString errorString = archive->errorString().isEmpty()
            ? Backend::fallbackMediaEntrySourceOpenError(openedCollectionScope)
            : archive->errorString();
        return OpenKArchiveResult { {}, errorString };
    }

    return OpenKArchiveResult { ScopedKArchive(std::move(archive)), QString() };
}

kiriview::MediaEntrySourceImageDataResult readKArchiveFileData(
    const kiriview::OpenedCollectionScopeLocation& openedCollectionScope, const QString& entryPath,
    const KArchiveFile& file)
{
    std::unique_ptr<QIODevice> device(file.createDevice());
    if (device == nullptr) {
        return Backend::mediaEntrySourceErrorResult<kiriview::MediaEntrySourceImageDataResult>(
            Backend::mediaEntrySourceError(kiriview::MediaEntrySourceBackendKind::KArchive,
                kiriview::MediaEntrySourceOperation::ReadImageData, openedCollectionScope,
                Backend::openedCollectionImageReadError(), {}, entryPath));
    }

    QByteArray data = device->readAll();
    const qint64 expectedSize = file.size();
    if (expectedSize >= 0 && data.size() != expectedSize) {
        return Backend::mediaEntrySourceErrorResult<kiriview::MediaEntrySourceImageDataResult>(
            Backend::mediaEntrySourceError(kiriview::MediaEntrySourceBackendKind::KArchive,
                kiriview::MediaEntrySourceOperation::ReadImageData, openedCollectionScope,
                Backend::openedCollectionImageReadError(), {}, entryPath));
    }

    return Backend::mediaEntrySourceImageDataResult(std::move(data));
}

std::optional<kiriview::MediaEntrySourceThumbnailMetadata> thumbnailMetadataForKArchiveFile(
    const KArchiveFile& file)
{
    const auto* zipFile = dynamic_cast<const KZipFileEntry*>(&file);
    if (zipFile == nullptr || file.size() < 0) {
        return std::nullopt;
    }

    return kiriview::MediaEntrySourceThumbnailMetadata {
        kiriview::MediaEntryContentChecksum {
            kiriview::MediaEntryContentChecksumAlgorithm::Crc32,
            zipFile->crc32(),
        },
        file.size(),
    };
}

class KArchiveMediaEntrySource final : public Backend::MediaEntrySourceWithCandidateSnapshot
{
public:
    KArchiveMediaEntrySource(kiriview::OpenedCollectionScopeLocation openedCollectionScope,
        ScopedKArchive archive, std::vector<kiriview::ImageDocumentPageCandidate> candidates)
        : Backend::MediaEntrySourceWithCandidateSnapshot(std::move(candidates))
        , m_openedCollectionScope(std::move(openedCollectionScope))
        , m_archive(std::move(archive))
    {
    }

    kiriview::MediaEntrySourceImageDataResult loadImageData(const QUrl& imageUrl) override
    {
        const std::optional<QString> entryPath
            = Backend::openedCollectionImageEntryPathForRead(m_openedCollectionScope, imageUrl);
        if (!entryPath.has_value()) {
            return Backend::mediaEntrySourceErrorResult<kiriview::MediaEntrySourceImageDataResult>(
                Backend::mediaEntrySourceError(kiriview::MediaEntrySourceBackendKind::KArchive,
                    kiriview::MediaEntrySourceOperation::ReadImageData, m_openedCollectionScope,
                    Backend::openedCollectionImageNotFoundError(), imageUrl.toString()));
        }

        const KArchiveDirectory* directory = m_archive.directory();
        const KArchiveFile* file = directory == nullptr ? nullptr : directory->file(*entryPath);
        if (file == nullptr) {
            return Backend::mediaEntrySourceErrorResult<kiriview::MediaEntrySourceImageDataResult>(
                Backend::mediaEntrySourceError(kiriview::MediaEntrySourceBackendKind::KArchive,
                    kiriview::MediaEntrySourceOperation::ReadImageData, m_openedCollectionScope,
                    Backend::openedCollectionImageNotFoundError(), {}, *entryPath));
        }

        return readKArchiveFileData(m_openedCollectionScope, *entryPath, *file);
    }

    kiriview::MediaEntrySourceThumbnailMetadataResult loadThumbnailMetadata(
        const QUrl& imageUrl) override
    {
        const std::optional<QString> entryPath
            = Backend::openedCollectionImageEntryPathForRead(m_openedCollectionScope, imageUrl);
        if (!entryPath.has_value()) {
            return Backend::mediaEntrySourceErrorResult<
                kiriview::MediaEntrySourceThumbnailMetadataResult>(Backend::mediaEntrySourceError(
                kiriview::MediaEntrySourceBackendKind::KArchive,
                kiriview::MediaEntrySourceOperation::LoadThumbnailMetadata, m_openedCollectionScope,
                Backend::openedCollectionImageNotFoundError(), imageUrl.toString()));
        }

        if (!kiriview::openedCollectionEntryPathSupportsThumbnailContentIdentity(
                m_openedCollectionScope, *entryPath)) {
            return Backend::mediaEntrySourceErrorResult<
                kiriview::MediaEntrySourceThumbnailMetadataResult>(Backend::mediaEntrySourceError(
                kiriview::MediaEntrySourceBackendKind::KArchive,
                kiriview::MediaEntrySourceOperation::LoadThumbnailMetadata, m_openedCollectionScope,
                Backend::openedCollectionThumbnailMetadataUnsupportedError(), {}, *entryPath));
        }

        const KArchiveDirectory* directory = m_archive.directory();
        const KArchiveFile* file = directory == nullptr ? nullptr : directory->file(*entryPath);
        if (file == nullptr) {
            return Backend::mediaEntrySourceErrorResult<
                kiriview::MediaEntrySourceThumbnailMetadataResult>(Backend::mediaEntrySourceError(
                kiriview::MediaEntrySourceBackendKind::KArchive,
                kiriview::MediaEntrySourceOperation::LoadThumbnailMetadata, m_openedCollectionScope,
                Backend::openedCollectionImageNotFoundError(), {}, *entryPath));
        }

        const std::optional<kiriview::MediaEntrySourceThumbnailMetadata> metadata
            = thumbnailMetadataForKArchiveFile(*file);
        if (!metadata.has_value()) {
            return Backend::mediaEntrySourceErrorResult<
                kiriview::MediaEntrySourceThumbnailMetadataResult>(Backend::mediaEntrySourceError(
                kiriview::MediaEntrySourceBackendKind::KArchive,
                kiriview::MediaEntrySourceOperation::LoadThumbnailMetadata, m_openedCollectionScope,
                Backend::openedCollectionThumbnailMetadataUnsupportedError(), {}, *entryPath));
        }

        return Backend::mediaEntrySourceThumbnailMetadataResult(*metadata);
    }

private:
    kiriview::OpenedCollectionScopeLocation m_openedCollectionScope;
    ScopedKArchive m_archive;
};

kiriview::MediaEntrySourceOpenResult openKArchiveMediaEntrySource(
    const kiriview::OpenedCollectionScopeLocation& openedCollectionScope)
{
    OpenKArchiveResult opened = openKArchiveCollection(openedCollectionScope);
    if (!opened.archive) {
        return Backend::mediaEntrySourceErrorResult<kiriview::MediaEntrySourceOpenResult>(
            Backend::mediaEntrySourceError(kiriview::MediaEntrySourceBackendKind::KArchive,
                kiriview::MediaEntrySourceOperation::OpenCollection, openedCollectionScope,
                opened.errorString));
    }

    std::vector<kiriview::ImageDocumentPageCandidate> candidates
        = archiveDirectoryImageDocumentPageCandidates(
            opened.archive.directory(), openedCollectionScope);
    return kiriview::MediaEntrySourcePtr(std::make_shared<KArchiveMediaEntrySource>(
        openedCollectionScope, std::move(opened.archive), std::move(candidates)));
}

}

namespace kiriview::MediaEntrySourceBackendDetail {
const MediaEntrySourceBackendOperations* kArchiveMediaEntrySourceBackendOperations()
{
    static const MediaEntrySourceBackendOperations operations {
        openKArchiveMediaEntrySource,
    };
    return &operations;
}
}
