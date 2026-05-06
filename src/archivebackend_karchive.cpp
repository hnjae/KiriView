// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivebackend_p.h"

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
namespace Backend = KiriView::ArchiveBackendDetail;

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

std::unique_ptr<KArchive> createArchive(const KiriView::ArchiveDocumentLocation &archiveDocument)
{
    const QString filePath = archiveDocument.fileUrl().toLocalFile();
    if (filePath.isEmpty()) {
        return nullptr;
    }

    switch (KiriView::archiveStorageBackendForRootScheme(archiveDocument.rootUrl().scheme())) {
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

void appendArchiveDirectoryImageCandidates(
    std::vector<KiriView::ImageNavigationCandidate> *candidates, const KArchiveDirectory *directory,
    const KiriView::ArchiveDocumentLocation &archiveDocument, const QString &prefix)
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
            appendArchiveDirectoryImageCandidates(candidates,
                static_cast<const KArchiveDirectory *>(entry), archiveDocument, entryPath);
            continue;
        }

        if (!entry->isFile()) {
            continue;
        }

        std::optional<KiriView::ImageNavigationCandidate> candidate
            = Backend::archiveImageCandidate(archiveDocument, entryPath);
        if (candidate.has_value()) {
            candidates->push_back(std::move(*candidate));
        }
    }
}

OpenKArchiveResult openKArchiveDocument(const KiriView::ArchiveDocumentLocation &archiveDocument)
{
    std::unique_ptr<KArchive> archive = createArchive(archiveDocument);
    if (archive == nullptr) {
        return OpenKArchiveResult { {}, Backend::fallbackArchiveOpenError(archiveDocument) };
    }

    if (!archive->open(QIODevice::ReadOnly)) {
        const QString errorString = archive->errorString().isEmpty()
            ? Backend::fallbackArchiveOpenError(archiveDocument)
            : archive->errorString();
        return OpenKArchiveResult { {}, errorString };
    }

    return OpenKArchiveResult { ScopedKArchive(std::move(archive)), QString() };
}

KiriView::ArchiveImageCandidatesResult loadKArchiveDocumentImageCandidates(
    const KiriView::ArchiveDocumentLocation &archiveDocument)
{
    OpenKArchiveResult opened = openKArchiveDocument(archiveDocument);
    if (!opened.archive) {
        return Backend::archiveErrorResult<KiriView::ArchiveImageCandidatesResult>(
            opened.errorString);
    }

    std::vector<KiriView::ImageNavigationCandidate> candidates;
    appendArchiveDirectoryImageCandidates(
        &candidates, opened.archive.directory(), archiveDocument, QString());
    return Backend::archiveImageCandidatesResult(std::move(candidates));
}

KiriView::ArchiveImageDataResult readKArchiveFileData(const KArchiveFile &file)
{
    std::unique_ptr<QIODevice> device(file.createDevice());
    if (device == nullptr) {
        return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
            Backend::archiveImageReadError());
    }

    QByteArray data = device->readAll();
    const qint64 expectedSize = file.size();
    if (expectedSize >= 0 && data.size() != expectedSize) {
        return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
            Backend::archiveImageReadError());
    }

    return Backend::archiveImageDataResult(std::move(data));
}

KiriView::ArchiveImageDataResult loadKArchiveDocumentImageData(
    const KiriView::ArchiveDocumentLocation &archiveDocument, const QString &entryPath)
{
    OpenKArchiveResult opened = openKArchiveDocument(archiveDocument);
    if (!opened.archive) {
        return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(opened.errorString);
    }

    const KArchiveDirectory *directory = opened.archive.directory();
    const KArchiveFile *file = directory == nullptr ? nullptr : directory->file(entryPath);
    if (file == nullptr) {
        return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
            Backend::archiveImageNotFoundError());
    }

    return readKArchiveFileData(*file);
}
}

namespace KiriView::ArchiveBackendDetail {
const ArchiveBackendOperations *kArchiveBackendOperations()
{
    static const ArchiveBackendOperations operations {
        loadKArchiveDocumentImageCandidates,
        loadKArchiveDocumentImageData,
    };
    return &operations;
}
}
