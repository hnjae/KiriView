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
namespace Backend = kiriview::MediaEntrySourceBackendDetail;

std::optional<QString> directoryPathForCollection(
    const kiriview::OpenedCollectionScopeLocation &openedCollectionScope)
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

kiriview::MediaEntrySourceCandidatesResult loadDirectoryCollectionImageDocumentPageCandidates(
    const kiriview::OpenedCollectionScopeLocation &openedCollectionScope)
{
    const std::optional<QString> directoryPath = directoryPathForCollection(openedCollectionScope);
    if (!directoryPath.has_value()) {
        return Backend::mediaEntrySourceErrorResult<kiriview::MediaEntrySourceCandidatesResult>(
            Backend::mediaEntrySourceError(kiriview::MediaEntrySourceBackendKind::Directory,
                kiriview::MediaEntrySourceOperation::ListCandidates, openedCollectionScope,
                Backend::fallbackMediaEntrySourceOpenError(openedCollectionScope)));
    }

    const QDir rootDirectory(*directoryPath);
    QDirIterator iterator(rootDirectory.absolutePath(),
        QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    std::vector<kiriview::ImageDocumentPageCandidate> candidates;
    while (iterator.hasNext()) {
        iterator.next();

        const QFileInfo fileInfo = iterator.fileInfo();
        if (!fileInfo.isFile()) {
            continue;
        }

        std::optional<kiriview::ImageDocumentPageCandidate> candidate
            = Backend::openedCollectionImageDocumentPageCandidate(
                openedCollectionScope, rootDirectory.relativeFilePath(fileInfo.filePath()));
        if (candidate.has_value()) {
            candidates.push_back(std::move(*candidate));
        }
    }

    return Backend::mediaEntrySourceCandidatesResult(std::move(candidates));
}

kiriview::MediaEntrySourceImageDataResult loadDirectoryCollectionImageData(
    const kiriview::OpenedCollectionScopeLocation &openedCollectionScope, const QString &entryPath)
{
    const std::optional<QString> directoryPath = directoryPathForCollection(openedCollectionScope);
    if (!directoryPath.has_value()) {
        return Backend::mediaEntrySourceErrorResult<kiriview::MediaEntrySourceImageDataResult>(
            Backend::mediaEntrySourceError(kiriview::MediaEntrySourceBackendKind::Directory,
                kiriview::MediaEntrySourceOperation::ReadImageData, openedCollectionScope,
                Backend::fallbackMediaEntrySourceOpenError(openedCollectionScope), {}, entryPath));
    }

    QFileInfo fileInfo(QDir(*directoryPath).filePath(entryPath));
    if (!fileInfo.isFile()) {
        return Backend::mediaEntrySourceErrorResult<kiriview::MediaEntrySourceImageDataResult>(
            Backend::mediaEntrySourceError(kiriview::MediaEntrySourceBackendKind::Directory,
                kiriview::MediaEntrySourceOperation::ReadImageData, openedCollectionScope,
                Backend::openedCollectionImageNotFoundError(), fileInfo.filePath(), entryPath));
    }

    QFile file(fileInfo.filePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return Backend::mediaEntrySourceErrorResult<kiriview::MediaEntrySourceImageDataResult>(
            Backend::mediaEntrySourceError(kiriview::MediaEntrySourceBackendKind::Directory,
                kiriview::MediaEntrySourceOperation::ReadImageData, openedCollectionScope,
                Backend::openedCollectionImageReadError(), file.errorString(), entryPath));
    }

    QByteArray data = file.readAll();
    if (file.error() != QFile::NoError || data.size() != fileInfo.size()) {
        return Backend::mediaEntrySourceErrorResult<kiriview::MediaEntrySourceImageDataResult>(
            Backend::mediaEntrySourceError(kiriview::MediaEntrySourceBackendKind::Directory,
                kiriview::MediaEntrySourceOperation::ReadImageData, openedCollectionScope,
                Backend::openedCollectionImageReadError(), file.errorString(), entryPath));
    }

    return Backend::mediaEntrySourceImageDataResult(std::move(data));
}

class DirectoryCollectionMediaEntrySource final
    : public Backend::MediaEntrySourceWithCandidateSnapshot
{
public:
    DirectoryCollectionMediaEntrySource(
        kiriview::OpenedCollectionScopeLocation openedCollectionScope,
        std::vector<kiriview::ImageDocumentPageCandidate> candidates)
        : Backend::MediaEntrySourceWithCandidateSnapshot(std::move(candidates))
        , m_openedCollectionScope(std::move(openedCollectionScope))
    {
    }

    kiriview::MediaEntrySourceImageDataResult loadImageData(const QUrl &imageUrl) override
    {
        const std::optional<QString> entryPath
            = Backend::openedCollectionImageEntryPathForRead(m_openedCollectionScope, imageUrl);
        if (!entryPath.has_value()) {
            return Backend::mediaEntrySourceErrorResult<kiriview::MediaEntrySourceImageDataResult>(
                Backend::mediaEntrySourceError(kiriview::MediaEntrySourceBackendKind::Directory,
                    kiriview::MediaEntrySourceOperation::ReadImageData, m_openedCollectionScope,
                    Backend::openedCollectionImageNotFoundError(), imageUrl.toString()));
        }

        return loadDirectoryCollectionImageData(m_openedCollectionScope, *entryPath);
    }

private:
    kiriview::OpenedCollectionScopeLocation m_openedCollectionScope;
};

kiriview::MediaEntrySourceOpenResult openDirectoryCollectionMediaEntrySource(
    const kiriview::OpenedCollectionScopeLocation &openedCollectionScope)
{
    kiriview::MediaEntrySourceCandidatesResult candidatesResult
        = loadDirectoryCollectionImageDocumentPageCandidates(openedCollectionScope);
    if (const auto *error = std::get_if<kiriview::MediaEntrySourceError>(&candidatesResult)) {
        return Backend::mediaEntrySourceErrorResult<kiriview::MediaEntrySourceOpenResult>(*error);
    }

    const auto *candidates = std::get_if<kiriview::MediaEntrySourceCandidates>(&candidatesResult);
    if (candidates == nullptr) {
        return Backend::mediaEntrySourceErrorResult<kiriview::MediaEntrySourceOpenResult>(
            Backend::mediaEntrySourceError(kiriview::MediaEntrySourceBackendKind::Directory,
                kiriview::MediaEntrySourceOperation::OpenCollection, openedCollectionScope,
                Backend::fallbackMediaEntrySourceOpenError(openedCollectionScope)));
    }

    return kiriview::MediaEntrySourcePtr(std::make_shared<DirectoryCollectionMediaEntrySource>(
        openedCollectionScope, candidates->candidates));
}
}

namespace kiriview::MediaEntrySourceBackendDetail {
const MediaEntrySourceBackendOperations *directoryCollectionMediaEntrySourceBackendOperations()
{
    static const MediaEntrySourceBackendOperations operations {
        openDirectoryCollectionMediaEntrySource,
    };
    return &operations;
}
}
