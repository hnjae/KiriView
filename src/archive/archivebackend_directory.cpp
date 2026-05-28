// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivebackend_p.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace {
namespace Backend = KiriView::ArchiveBackendDetail;

std::optional<QString> directoryPathForCollection(
    const KiriView::OpenedCollectionScopeLocation &archiveCollection)
{
    if (!archiveCollection.isDirectory()) {
        return std::nullopt;
    }

    const QString path = archiveCollection.fileUrl().toLocalFile();
    if (path.isEmpty() || !QFileInfo(path).isDir()) {
        return std::nullopt;
    }

    return path;
}

KiriView::ArchiveImageCandidatesResult loadDirectoryCollectionImageCandidates(
    const KiriView::OpenedCollectionScopeLocation &archiveCollection)
{
    const std::optional<QString> directoryPath = directoryPathForCollection(archiveCollection);
    if (!directoryPath.has_value()) {
        return Backend::archiveErrorResult<KiriView::ArchiveImageCandidatesResult>(
            Backend::fallbackArchiveOpenError(archiveCollection));
    }

    const QDir rootDirectory(*directoryPath);
    QDirIterator iterator(rootDirectory.absolutePath(),
        QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    std::vector<KiriView::ImageNavigationCandidate> candidates;
    while (iterator.hasNext()) {
        iterator.next();

        const QFileInfo fileInfo = iterator.fileInfo();
        if (!fileInfo.isFile()) {
            continue;
        }

        std::optional<KiriView::ImageNavigationCandidate> candidate
            = Backend::archiveMediaCandidate(
                archiveCollection, rootDirectory.relativeFilePath(fileInfo.filePath()));
        if (candidate.has_value()) {
            candidates.push_back(std::move(*candidate));
        }
    }

    return Backend::archiveImageCandidatesResult(std::move(candidates));
}

KiriView::ArchiveImageDataResult loadDirectoryCollectionImageData(
    const KiriView::OpenedCollectionScopeLocation &archiveCollection, const QString &entryPath)
{
    const std::optional<QString> directoryPath = directoryPathForCollection(archiveCollection);
    if (!directoryPath.has_value()) {
        return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
            Backend::fallbackArchiveOpenError(archiveCollection));
    }

    QFileInfo fileInfo(QDir(*directoryPath).filePath(entryPath));
    if (!fileInfo.isFile()) {
        return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
            Backend::archiveImageNotFoundError());
    }

    QFile file(fileInfo.filePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
            Backend::archiveImageReadError());
    }

    QByteArray data = file.readAll();
    if (file.error() != QFile::NoError || data.size() != fileInfo.size()) {
        return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
            Backend::archiveImageReadError());
    }

    return Backend::archiveImageDataResult(std::move(data));
}

class DirectoryMediaEntrySource final : public Backend::MediaEntrySourceWithCandidateSnapshot
{
public:
    DirectoryMediaEntrySource(KiriView::OpenedCollectionScopeLocation archiveCollection,
        std::vector<KiriView::ImageNavigationCandidate> candidates)
        : Backend::MediaEntrySourceWithCandidateSnapshot(std::move(candidates))
        , m_archiveCollection(std::move(archiveCollection))
    {
    }

    KiriView::ArchiveImageDataResult loadImageData(const QUrl &imageUrl) override
    {
        const std::optional<QString> entryPath
            = Backend::archiveImageEntryPathForRead(m_archiveCollection, imageUrl);
        if (!entryPath.has_value()) {
            return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
                Backend::archiveImageNotFoundError());
        }

        return loadDirectoryCollectionImageData(m_archiveCollection, *entryPath);
    }

private:
    KiriView::OpenedCollectionScopeLocation m_archiveCollection;
};

KiriView::MediaEntrySourceOpenResult openDirectoryMediaEntrySource(
    const KiriView::OpenedCollectionScopeLocation &archiveCollection)
{
    KiriView::ArchiveImageCandidatesResult candidatesResult
        = loadDirectoryCollectionImageCandidates(archiveCollection);
    if (const auto *error = std::get_if<KiriView::ArchiveError>(&candidatesResult)) {
        return Backend::archiveErrorResult<KiriView::MediaEntrySourceOpenResult>(
            error->errorString);
    }

    const auto *candidates = std::get_if<KiriView::ArchiveImageCandidates>(&candidatesResult);
    if (candidates == nullptr) {
        return Backend::archiveErrorResult<KiriView::MediaEntrySourceOpenResult>(
            Backend::fallbackArchiveOpenError(archiveCollection));
    }

    return KiriView::MediaEntrySourcePtr(
        std::make_shared<DirectoryMediaEntrySource>(archiveCollection, candidates->candidates));
}
}

namespace KiriView::ArchiveBackendDetail {
const ArchiveBackendOperations *directoryBackendOperations()
{
    static const ArchiveBackendOperations operations {
        openDirectoryMediaEntrySource,
    };
    return &operations;
}
}
