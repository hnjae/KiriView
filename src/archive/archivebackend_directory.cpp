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

std::optional<QString> directoryPathForDocument(
    const KiriView::ImagePageScopeLocation &archiveDocument)
{
    if (!archiveDocument.isDirectory()) {
        return std::nullopt;
    }

    const QString path = archiveDocument.fileUrl().toLocalFile();
    if (path.isEmpty() || !QFileInfo(path).isDir()) {
        return std::nullopt;
    }

    return path;
}

KiriView::ArchiveImageCandidatesResult loadDirectoryDocumentImageCandidates(
    const KiriView::ImagePageScopeLocation &archiveDocument)
{
    const std::optional<QString> directoryPath = directoryPathForDocument(archiveDocument);
    if (!directoryPath.has_value()) {
        return Backend::archiveErrorResult<KiriView::ArchiveImageCandidatesResult>(
            Backend::fallbackArchiveOpenError(archiveDocument));
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
            = Backend::archiveImageCandidate(
                archiveDocument, rootDirectory.relativeFilePath(fileInfo.filePath()));
        if (candidate.has_value()) {
            candidates.push_back(std::move(*candidate));
        }
    }

    return Backend::archiveImageCandidatesResult(std::move(candidates));
}

KiriView::ArchiveImageDataResult loadDirectoryDocumentImageData(
    const KiriView::ImagePageScopeLocation &archiveDocument, const QString &entryPath)
{
    const std::optional<QString> directoryPath = directoryPathForDocument(archiveDocument);
    if (!directoryPath.has_value()) {
        return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
            Backend::fallbackArchiveOpenError(archiveDocument));
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

class DirectoryDocumentSession final : public Backend::ArchiveDocumentSessionWithCandidateSnapshot
{
public:
    DirectoryDocumentSession(KiriView::ImagePageScopeLocation archiveDocument,
        std::vector<KiriView::ImageNavigationCandidate> candidates)
        : Backend::ArchiveDocumentSessionWithCandidateSnapshot(std::move(candidates))
        , m_archiveDocument(std::move(archiveDocument))
    {
    }

    KiriView::ArchiveImageDataResult loadImageData(const QUrl &imageUrl) override
    {
        const std::optional<QString> entryPath
            = Backend::archiveImageEntryPathForRead(m_archiveDocument, imageUrl);
        if (!entryPath.has_value()) {
            return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
                Backend::archiveImageNotFoundError());
        }

        return loadDirectoryDocumentImageData(m_archiveDocument, *entryPath);
    }

private:
    KiriView::ImagePageScopeLocation m_archiveDocument;
};

KiriView::ArchiveDocumentSessionOpenResult openDirectoryDocumentSession(
    const KiriView::ImagePageScopeLocation &archiveDocument)
{
    KiriView::ArchiveImageCandidatesResult candidatesResult
        = loadDirectoryDocumentImageCandidates(archiveDocument);
    if (const auto *error = std::get_if<KiriView::ArchiveError>(&candidatesResult)) {
        return Backend::archiveErrorResult<KiriView::ArchiveDocumentSessionOpenResult>(
            error->errorString);
    }

    const auto *candidates = std::get_if<KiriView::ArchiveImageCandidates>(&candidatesResult);
    if (candidates == nullptr) {
        return Backend::archiveErrorResult<KiriView::ArchiveDocumentSessionOpenResult>(
            Backend::fallbackArchiveOpenError(archiveDocument));
    }

    return KiriView::ArchiveDocumentSessionPtr(
        std::make_shared<DirectoryDocumentSession>(archiveDocument, candidates->candidates));
}
}

namespace KiriView::ArchiveBackendDetail {
const ArchiveBackendOperations *directoryBackendOperations()
{
    static const ArchiveBackendOperations operations {
        openDirectoryDocumentSession,
    };
    return &operations;
}
}
