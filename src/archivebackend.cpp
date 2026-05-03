// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivebackend.h"

#include "archiveformat.h"
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
#include <QFile>
#include <QIODevice>
#include <archive.h>
#include <archive_entry.h>
#include <cstddef>
#include <memory>

namespace {
using KiriView::ArchiveImageCandidatesResult;
using KiriView::ArchiveImageDataResult;
using LibArchiveReader = std::unique_ptr<archive, int (*)(archive *)>;

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

bool isLibArchiveDocument(const KiriView::ArchiveDocumentLocation &archiveDocument)
{
    return KiriView::archiveStorageBackendForRootScheme(archiveDocument.rootUrl().scheme())
        == KiriView::ArchiveStorageBackend::LibArchive;
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

QString libArchiveErrorString(archive *reader, const QString &fallback)
{
    const char *message = reader == nullptr ? nullptr : archive_error_string(reader);
    if (message == nullptr || message[0] == '\0') {
        return fallback;
    }

    return QString::fromLocal8Bit(message);
}

LibArchiveReader makeLibArchiveReader()
{
    return LibArchiveReader(archive_read_new(), archive_read_free);
}

LibArchiveReader openLibArchiveReader(
    const KiriView::ArchiveDocumentLocation &archiveDocument, QString *errorString)
{
    const QString filePath = archiveDocument.fileUrl().toLocalFile();
    if (filePath.isEmpty()) {
        *errorString = fallbackArchiveOpenError(archiveDocument);
        return LibArchiveReader(nullptr, archive_read_free);
    }

    LibArchiveReader reader = makeLibArchiveReader();
    if (reader == nullptr) {
        *errorString = fallbackArchiveOpenError(archiveDocument);
        return reader;
    }

    if (archive_read_support_filter_all(reader.get()) != ARCHIVE_OK
        || archive_read_support_format_rar(reader.get()) != ARCHIVE_OK
        || archive_read_support_format_rar5(reader.get()) != ARCHIVE_OK) {
        *errorString
            = libArchiveErrorString(reader.get(), fallbackArchiveOpenError(archiveDocument));
        return LibArchiveReader(nullptr, archive_read_free);
    }

    const QByteArray encodedFilePath = QFile::encodeName(filePath);
    if (archive_read_open_filename(reader.get(), encodedFilePath.constData(), 10240)
        != ARCHIVE_OK) {
        *errorString
            = libArchiveErrorString(reader.get(), fallbackArchiveOpenError(archiveDocument));
        return LibArchiveReader(nullptr, archive_read_free);
    }

    return reader;
}

QString libArchiveEntryPath(archive_entry *entry)
{
    const char *utf8Path = archive_entry_pathname_utf8(entry);
    if (utf8Path != nullptr) {
        return QString::fromUtf8(utf8Path);
    }

    const char *path = archive_entry_pathname(entry);
    if (path == nullptr) {
        return {};
    }

    return QFile::decodeName(path);
}

bool skipLibArchiveEntry(archive *reader, QString *errorString)
{
    if (archive_read_data_skip(reader) == ARCHIVE_OK) {
        return true;
    }

    *errorString = libArchiveErrorString(
        reader, QStringLiteral("Could not read the selected archive image."));
    return false;
}

ArchiveImageCandidatesResult loadLibArchiveDocumentImageCandidates(
    const KiriView::ArchiveDocumentLocation &archiveDocument)
{
    QString errorString;
    LibArchiveReader reader = openLibArchiveReader(archiveDocument, &errorString);
    if (reader == nullptr) {
        return ArchiveImageCandidatesResult { {}, errorString, false };
    }

    std::vector<KiriView::ImageNavigationCandidate> candidates;
    archive_entry *entry = nullptr;
    while (true) {
        const int status = archive_read_next_header(reader.get(), &entry);
        if (status == ARCHIVE_EOF) {
            break;
        }
        if (status != ARCHIVE_OK) {
            return ArchiveImageCandidatesResult {
                {},
                libArchiveErrorString(reader.get(), fallbackArchiveOpenError(archiveDocument)),
                false,
            };
        }

        const QString entryPath = normalizeEntryPath(libArchiveEntryPath(entry));
        if (archive_entry_filetype(entry) == AE_IFREG && !entryPath.isEmpty()
            && KiriView::isSupportedImageFileName(entryPath)) {
            const QUrl url = archiveEntryUrl(archiveDocument, entryPath);
            if (!url.isEmpty()) {
                candidates.push_back(KiriView::ImageNavigationCandidate { url, entryPath });
            }
        }

        if (!skipLibArchiveEntry(reader.get(), &errorString)) {
            return ArchiveImageCandidatesResult { {}, errorString, false };
        }
    }

    KiriView::sortImageNavigationCandidates(&candidates);
    return ArchiveImageCandidatesResult { std::move(candidates), QString(), true };
}

ArchiveImageDataResult readLibArchiveEntryData(archive *reader, archive_entry *entry)
{
    QByteArray data;
    char buffer[64 * 1024];
    while (true) {
        const la_ssize_t bytesRead = archive_read_data(reader, buffer, sizeof(buffer));
        if (bytesRead == 0) {
            break;
        }
        if (bytesRead < 0) {
            return ArchiveImageDataResult {
                {},
                libArchiveErrorString(
                    reader, QStringLiteral("Could not read the selected archive image.")),
                false,
            };
        }

        data.append(buffer, static_cast<qsizetype>(bytesRead));
    }

    if (archive_entry_size_is_set(entry)) {
        const la_int64_t expectedSize = archive_entry_size(entry);
        if (expectedSize >= 0 && static_cast<qint64>(data.size()) != expectedSize) {
            return ArchiveImageDataResult {
                {},
                QStringLiteral("Could not read the selected archive image."),
                false,
            };
        }
    }

    return ArchiveImageDataResult { std::move(data), QString(), true };
}

ArchiveImageDataResult loadLibArchiveDocumentImageData(
    const KiriView::ArchiveDocumentLocation &archiveDocument, const QUrl &imageUrl)
{
    const QString entryPath = archiveEntryPathForUrl(archiveDocument, imageUrl);
    if (archiveDocument.isEmpty() || entryPath.isEmpty()) {
        return ArchiveImageDataResult {
            {},
            QStringLiteral("Could not find the selected image in the archive."),
            false,
        };
    }

    QString errorString;
    LibArchiveReader reader = openLibArchiveReader(archiveDocument, &errorString);
    if (reader == nullptr) {
        return ArchiveImageDataResult { {}, errorString, false };
    }

    archive_entry *entry = nullptr;
    while (true) {
        const int status = archive_read_next_header(reader.get(), &entry);
        if (status == ARCHIVE_EOF) {
            break;
        }
        if (status != ARCHIVE_OK) {
            return ArchiveImageDataResult {
                {},
                libArchiveErrorString(reader.get(), fallbackArchiveOpenError(archiveDocument)),
                false,
            };
        }

        const QString currentPath = normalizeEntryPath(libArchiveEntryPath(entry));
        if (archive_entry_filetype(entry) == AE_IFREG && currentPath == entryPath) {
            return readLibArchiveEntryData(reader.get(), entry);
        }

        if (!skipLibArchiveEntry(reader.get(), &errorString)) {
            return ArchiveImageDataResult { {}, errorString, false };
        }
    }

    return ArchiveImageDataResult {
        {},
        QStringLiteral("Could not find the selected image in the archive."),
        false,
    };
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

    if (isLibArchiveDocument(archiveDocument)) {
        return loadLibArchiveDocumentImageCandidates(archiveDocument);
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

    if (isLibArchiveDocument(archiveDocument)) {
        return loadLibArchiveDocumentImageData(archiveDocument, imageUrl);
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
