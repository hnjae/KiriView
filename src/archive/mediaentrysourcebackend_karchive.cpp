// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaentrysourcebackend_p.h"

#include "archiveformat.h"

#include <K7Zip>
#include <KArchive>
#include <KArchiveDirectory>
#include <KArchiveFile>
#include <KTar>
#include <KZip>
#include <QIODevice>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
namespace Backend = KiriView::MediaEntrySourceBackendDetail;

class ScopedKArchive final
{
public:
    ScopedKArchive() = default;

    explicit ScopedKArchive(std::unique_ptr<KArchive> archive)
        : m_archive(std::move(archive))
    {
    }

    ~ScopedKArchive() { close(); }

    ScopedKArchive(const ScopedKArchive &) = delete;
    ScopedKArchive &operator=(const ScopedKArchive &) = delete;
    ScopedKArchive(ScopedKArchive &&) noexcept = default;
    ScopedKArchive &operator=(ScopedKArchive &&other) noexcept
    {
        if (this == &other) {
            return *this;
        }

        close();
        m_archive = std::move(other.m_archive);
        return *this;
    }

    const KArchiveDirectory *directory() const
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

struct OpenKArchiveResult {
    ScopedKArchive archive;
    QString errorString;
};

std::unique_ptr<KArchive> createArchive(
    const KiriView::OpenedCollectionScopeLocation &openedCollectionScope)
{
    const QString filePath = openedCollectionScope.fileUrl().toLocalFile();
    if (filePath.isEmpty()) {
        return nullptr;
    }

    switch (
        KiriView::archiveStorageBackendForRootScheme(openedCollectionScope.rootUrl().scheme())) {
    case KiriView::ArchiveStorageBackend::KZip:
        return std::make_unique<KZip>(filePath);
    case KiriView::ArchiveStorageBackend::KTar:
        return std::make_unique<KTar>(filePath);
    case KiriView::ArchiveStorageBackend::K7Zip:
        return std::make_unique<K7Zip>(filePath);
    case KiriView::ArchiveStorageBackend::LibArchive:
    case KiriView::ArchiveStorageBackend::None:
        return nullptr;
    }

    return nullptr;
}

void appendArchiveDirectoryMediaCandidates(
    std::vector<KiriView::ImageNavigationCandidate> *candidates, const KArchiveDirectory *directory,
    const KiriView::OpenedCollectionScopeLocation &openedCollectionScope, const QString &prefix)
{
    if (directory == nullptr) {
        return;
    }

    const QStringList entries = directory->entries();
    candidates->reserve(candidates->size() + static_cast<std::size_t>(entries.size()));
    for (const QString &entryName : entries) {
        const QString entryPath
            = prefix.isEmpty() ? entryName : prefix + QLatin1Char('/') + entryName;
        const KArchiveEntry *entry = directory->entry(entryName);
        if (entry == nullptr) {
            continue;
        }

        if (entry->isDirectory()) {
            appendArchiveDirectoryMediaCandidates(candidates,
                static_cast<const KArchiveDirectory *>(entry), openedCollectionScope, entryPath);
            continue;
        }

        if (!entry->isFile()) {
            continue;
        }

        std::optional<KiriView::ImageNavigationCandidate> candidate
            = Backend::openedCollectionMediaCandidate(openedCollectionScope, entryPath);
        if (candidate.has_value()) {
            candidates->push_back(std::move(*candidate));
        }
    }
}

std::vector<KiriView::ImageNavigationCandidate> archiveDirectoryImageCandidates(
    const KArchiveDirectory *directory,
    const KiriView::OpenedCollectionScopeLocation &openedCollectionScope)
{
    std::vector<KiriView::ImageNavigationCandidate> candidates;
    appendArchiveDirectoryMediaCandidates(&candidates, directory, openedCollectionScope, QString());
    return candidates;
}

OpenKArchiveResult openKArchiveCollection(
    const KiriView::OpenedCollectionScopeLocation &openedCollectionScope)
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

KiriView::MediaEntrySourceImageDataResult readKArchiveFileData(const KArchiveFile &file)
{
    std::unique_ptr<QIODevice> device(file.createDevice());
    if (device == nullptr) {
        return Backend::mediaEntrySourceErrorResult<KiriView::MediaEntrySourceImageDataResult>(
            Backend::openedCollectionImageReadError());
    }

    QByteArray data = device->readAll();
    const qint64 expectedSize = file.size();
    if (expectedSize >= 0 && data.size() != expectedSize) {
        return Backend::mediaEntrySourceErrorResult<KiriView::MediaEntrySourceImageDataResult>(
            Backend::openedCollectionImageReadError());
    }

    return Backend::mediaEntrySourceImageDataResult(std::move(data));
}

class KArchiveMediaEntrySource final : public Backend::MediaEntrySourceWithCandidateSnapshot
{
public:
    KArchiveMediaEntrySource(KiriView::OpenedCollectionScopeLocation openedCollectionScope,
        ScopedKArchive archive, std::vector<KiriView::ImageNavigationCandidate> candidates)
        : Backend::MediaEntrySourceWithCandidateSnapshot(std::move(candidates))
        , m_openedCollectionScope(std::move(openedCollectionScope))
        , m_archive(std::move(archive))
    {
    }

    KiriView::MediaEntrySourceImageDataResult loadImageData(const QUrl &imageUrl) override
    {
        const std::optional<QString> entryPath
            = Backend::openedCollectionImageEntryPathForRead(m_openedCollectionScope, imageUrl);
        if (!entryPath.has_value()) {
            return Backend::mediaEntrySourceErrorResult<KiriView::MediaEntrySourceImageDataResult>(
                Backend::openedCollectionImageNotFoundError());
        }

        const KArchiveDirectory *directory = m_archive.directory();
        const KArchiveFile *file = directory == nullptr ? nullptr : directory->file(*entryPath);
        if (file == nullptr) {
            return Backend::mediaEntrySourceErrorResult<KiriView::MediaEntrySourceImageDataResult>(
                Backend::openedCollectionImageNotFoundError());
        }

        return readKArchiveFileData(*file);
    }

private:
    KiriView::OpenedCollectionScopeLocation m_openedCollectionScope;
    ScopedKArchive m_archive;
};

KiriView::MediaEntrySourceOpenResult openKArchiveMediaEntrySource(
    const KiriView::OpenedCollectionScopeLocation &openedCollectionScope)
{
    OpenKArchiveResult opened = openKArchiveCollection(openedCollectionScope);
    if (!opened.archive) {
        return Backend::mediaEntrySourceErrorResult<KiriView::MediaEntrySourceOpenResult>(
            opened.errorString);
    }

    std::vector<KiriView::ImageNavigationCandidate> candidates
        = archiveDirectoryImageCandidates(opened.archive.directory(), openedCollectionScope);
    return KiriView::MediaEntrySourcePtr(std::make_shared<KArchiveMediaEntrySource>(
        openedCollectionScope, std::move(opened.archive), std::move(candidates)));
}

}

namespace KiriView::MediaEntrySourceBackendDetail {
const MediaEntrySourceBackendOperations *kArchiveMediaEntrySourceBackendOperations()
{
    static const MediaEntrySourceBackendOperations operations {
        openKArchiveMediaEntrySource,
    };
    return &operations;
}
}
