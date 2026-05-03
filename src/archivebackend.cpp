// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivebackend.h"

#include "imagecontainer.h"
#include "imageformatregistry.h"
#include "imagenavigationmodel.h"

#include <K7Zip>
#include <KArchive>
#include <KArchiveDirectory>
#include <KArchiveFile>
#include <KTar>
#include <KZip>
#include <QDir>
#include <QIODevice>
#include <cstddef>
#include <memory>

namespace {
std::unique_ptr<KArchive> createArchive(const KiriView::ArchiveDocumentLocation &archiveDocument)
{
    const QString filePath = archiveDocument.fileUrl().toLocalFile();
    if (filePath.isEmpty()) {
        return nullptr;
    }

    const QString scheme = archiveDocument.rootUrl().scheme();
    if (scheme == QStringLiteral("zip")) {
        return std::make_unique<KZip>(filePath);
    }
    if (scheme == QStringLiteral("tar")) {
        return std::make_unique<KTar>(filePath);
    }
    if (scheme == QStringLiteral("sevenz")) {
        return std::make_unique<K7Zip>(filePath);
    }

    return nullptr;
}

QString normalizeEntryPath(const QString &path)
{
    QString cleanPath = QDir::cleanPath(path);
    while (cleanPath.startsWith(QStringLiteral("./"))) {
        cleanPath.remove(0, 2);
    }
    if (cleanPath == QStringLiteral(".") || cleanPath == QStringLiteral("..")
        || cleanPath.startsWith(QStringLiteral("../")) || cleanPath.startsWith(QLatin1Char('/'))) {
        return {};
    }

    return cleanPath;
}

QUrl archiveEntryUrl(
    const KiriView::ArchiveDocumentLocation &archiveDocument, const QString &entryPath)
{
    const QString cleanEntryPath = normalizeEntryPath(entryPath);
    if (cleanEntryPath.isEmpty()) {
        return {};
    }

    QUrl url = archiveDocument.rootUrl();
    QString rootPath = QDir::cleanPath(url.path());
    if (!rootPath.endsWith(QLatin1Char('/'))) {
        rootPath += QLatin1Char('/');
    }
    url.setPath(rootPath + cleanEntryPath);
    url.setQuery(QString());
    url.setFragment(QString());
    return url;
}

QString archiveEntryPathForUrl(
    const KiriView::ArchiveDocumentLocation &archiveDocument, const QUrl &imageUrl)
{
    if (!KiriView::isUrlInsideArchiveRoot(imageUrl, archiveDocument.rootUrl())) {
        return {};
    }

    QString rootPath = QDir::cleanPath(archiveDocument.rootUrl().path());
    if (!rootPath.endsWith(QLatin1Char('/'))) {
        rootPath += QLatin1Char('/');
    }

    const QString path = QDir::cleanPath(imageUrl.path());
    if (!path.startsWith(rootPath)) {
        return {};
    }

    return normalizeEntryPath(path.mid(rootPath.size()));
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

        if (!entry->isFile() || !KiriView::isSupportedImageFileName(entry->name())) {
            continue;
        }

        const QString candidateName = normalizeEntryPath(entryPath);
        const QUrl url = archiveEntryUrl(archiveDocument, candidateName);
        if (candidateName.isEmpty() || url.isEmpty()) {
            continue;
        }

        candidates->push_back(KiriView::ImageNavigationCandidate { url, candidateName });
    }
}

QString fallbackArchiveOpenError(const KiriView::ArchiveDocumentLocation &archiveDocument)
{
    const QString fileName = archiveDocument.fileUrl().fileName();
    if (fileName.isEmpty()) {
        return QStringLiteral("Could not open the selected archive.");
    }

    return QStringLiteral("Could not open %1.").arg(fileName);
}
}

namespace KiriView {
ArchiveImageCandidatesResult loadArchiveDocumentImageCandidates(
    const ArchiveDocumentLocation &archiveDocument)
{
    if (archiveDocument.isEmpty()) {
        return ArchiveImageCandidatesResult {
            {},
            QStringLiteral("Could not open the selected archive."),
            false,
        };
    }

    std::unique_ptr<KArchive> archive = createArchive(archiveDocument);
    if (archive == nullptr) {
        return ArchiveImageCandidatesResult {
            {},
            fallbackArchiveOpenError(archiveDocument),
            false,
        };
    }

    if (!archive->open(QIODevice::ReadOnly)) {
        const QString errorString = archive->errorString().isEmpty()
            ? fallbackArchiveOpenError(archiveDocument)
            : archive->errorString();
        return ArchiveImageCandidatesResult { {}, errorString, false };
    }

    std::vector<ImageNavigationCandidate> candidates;
    appendArchiveDirectoryImageCandidates(
        &candidates, archive->directory(), archiveDocument, QString());
    sortImageNavigationCandidates(&candidates);
    archive->close();
    return ArchiveImageCandidatesResult { std::move(candidates), QString(), true };
}

ArchiveImageDataResult loadArchiveDocumentImageData(
    const ArchiveDocumentLocation &archiveDocument, const QUrl &imageUrl)
{
    const QString entryPath = archiveEntryPathForUrl(archiveDocument, imageUrl);
    if (archiveDocument.isEmpty() || entryPath.isEmpty()) {
        return ArchiveImageDataResult {
            {},
            QStringLiteral("Could not find the selected image in the archive."),
            false,
        };
    }

    std::unique_ptr<KArchive> archive = createArchive(archiveDocument);
    if (archive == nullptr) {
        return ArchiveImageDataResult { {}, fallbackArchiveOpenError(archiveDocument), false };
    }

    if (!archive->open(QIODevice::ReadOnly)) {
        const QString errorString = archive->errorString().isEmpty()
            ? fallbackArchiveOpenError(archiveDocument)
            : archive->errorString();
        return ArchiveImageDataResult { {}, errorString, false };
    }

    const KArchiveDirectory *directory = archive->directory();
    const KArchiveFile *file = directory == nullptr ? nullptr : directory->file(entryPath);
    if (file == nullptr) {
        archive->close();
        return ArchiveImageDataResult {
            {},
            QStringLiteral("Could not find the selected image in the archive."),
            false,
        };
    }

    std::unique_ptr<QIODevice> device(file->createDevice());
    if (device == nullptr) {
        archive->close();
        return ArchiveImageDataResult {
            {},
            QStringLiteral("Could not read the selected archive image."),
            false,
        };
    }

    QByteArray data = device->readAll();
    const qint64 expectedSize = file->size();
    archive->close();
    if (expectedSize >= 0 && data.size() != expectedSize) {
        return ArchiveImageDataResult {
            {},
            QStringLiteral("Could not read the selected archive image."),
            false,
        };
    }

    return ArchiveImageDataResult { std::move(data), QString(), true };
}
}
