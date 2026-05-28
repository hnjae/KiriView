// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaentrysourcebackend_p.h"

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
namespace Backend = KiriView::MediaEntrySourceBackendDetail;

std::optional<QString> directoryPathForCollection(
    const KiriView::OpenedCollectionScopeLocation &openedCollectionScope)
{
    if (!openedCollectionScope.isDirectory()) {
        return std::nullopt;
    }

    const QString path = openedCollectionScope.fileUrl().toLocalFile();
    if (path.isEmpty() || !QFileInfo(path).isDir()) {
        return std::nullopt;
    }

    return path;
}

KiriView::MediaEntrySourceCandidatesResult loadDirectoryCollectionImageDocumentPageCandidates(
    const KiriView::OpenedCollectionScopeLocation &openedCollectionScope)
{
    const std::optional<QString> directoryPath = directoryPathForCollection(openedCollectionScope);
    if (!directoryPath.has_value()) {
        return Backend::mediaEntrySourceErrorResult<KiriView::MediaEntrySourceCandidatesResult>(
            Backend::fallbackMediaEntrySourceOpenError(openedCollectionScope));
    }

    const QDir rootDirectory(*directoryPath);
    QDirIterator iterator(rootDirectory.absolutePath(),
        QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    std::vector<KiriView::ImageDocumentPageCandidate> candidates;
    while (iterator.hasNext()) {
        iterator.next();

        const QFileInfo fileInfo = iterator.fileInfo();
        if (!fileInfo.isFile()) {
            continue;
        }

        std::optional<KiriView::ImageDocumentPageCandidate> candidate
            = Backend::openedCollectionImageDocumentPageCandidate(
                openedCollectionScope, rootDirectory.relativeFilePath(fileInfo.filePath()));
        if (candidate.has_value()) {
            candidates.push_back(std::move(*candidate));
        }
    }

    return Backend::mediaEntrySourceCandidatesResult(std::move(candidates));
}

KiriView::MediaEntrySourceImageDataResult loadDirectoryCollectionImageData(
    const KiriView::OpenedCollectionScopeLocation &openedCollectionScope, const QString &entryPath)
{
    const std::optional<QString> directoryPath = directoryPathForCollection(openedCollectionScope);
    if (!directoryPath.has_value()) {
        return Backend::mediaEntrySourceErrorResult<KiriView::MediaEntrySourceImageDataResult>(
            Backend::fallbackMediaEntrySourceOpenError(openedCollectionScope));
    }

    QFileInfo fileInfo(QDir(*directoryPath).filePath(entryPath));
    if (!fileInfo.isFile()) {
        return Backend::mediaEntrySourceErrorResult<KiriView::MediaEntrySourceImageDataResult>(
            Backend::openedCollectionImageNotFoundError());
    }

    QFile file(fileInfo.filePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return Backend::mediaEntrySourceErrorResult<KiriView::MediaEntrySourceImageDataResult>(
            Backend::openedCollectionImageReadError());
    }

    QByteArray data = file.readAll();
    if (file.error() != QFile::NoError || data.size() != fileInfo.size()) {
        return Backend::mediaEntrySourceErrorResult<KiriView::MediaEntrySourceImageDataResult>(
            Backend::openedCollectionImageReadError());
    }

    return Backend::mediaEntrySourceImageDataResult(std::move(data));
}

class DirectoryCollectionMediaEntrySource final
    : public Backend::MediaEntrySourceWithCandidateSnapshot
{
public:
    DirectoryCollectionMediaEntrySource(
        KiriView::OpenedCollectionScopeLocation openedCollectionScope,
        std::vector<KiriView::ImageDocumentPageCandidate> candidates)
        : Backend::MediaEntrySourceWithCandidateSnapshot(std::move(candidates))
        , m_openedCollectionScope(std::move(openedCollectionScope))
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

        return loadDirectoryCollectionImageData(m_openedCollectionScope, *entryPath);
    }

private:
    KiriView::OpenedCollectionScopeLocation m_openedCollectionScope;
};

KiriView::MediaEntrySourceOpenResult openDirectoryCollectionMediaEntrySource(
    const KiriView::OpenedCollectionScopeLocation &openedCollectionScope)
{
    KiriView::MediaEntrySourceCandidatesResult candidatesResult
        = loadDirectoryCollectionImageDocumentPageCandidates(openedCollectionScope);
    if (const auto *error = std::get_if<KiriView::MediaEntrySourceError>(&candidatesResult)) {
        return Backend::mediaEntrySourceErrorResult<KiriView::MediaEntrySourceOpenResult>(
            error->errorString);
    }

    const auto *candidates = std::get_if<KiriView::MediaEntrySourceCandidates>(&candidatesResult);
    if (candidates == nullptr) {
        return Backend::mediaEntrySourceErrorResult<KiriView::MediaEntrySourceOpenResult>(
            Backend::fallbackMediaEntrySourceOpenError(openedCollectionScope));
    }

    return KiriView::MediaEntrySourcePtr(std::make_shared<DirectoryCollectionMediaEntrySource>(
        openedCollectionScope, candidates->candidates));
}
}

namespace KiriView::MediaEntrySourceBackendDetail {
const MediaEntrySourceBackendOperations *directoryCollectionMediaEntrySourceBackendOperations()
{
    static const MediaEntrySourceBackendOperations operations {
        openDirectoryCollectionMediaEntrySource,
    };
    return &operations;
}
}
