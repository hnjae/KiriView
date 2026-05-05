// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivebackend.h"

#include "archiveformat.h"
#include "archivepath.h"
#include "imageformatregistry.h"
#include "imagenavigationmodel.h"

#include <K7Zip>
#include <KArchive>
#include <KArchiveDirectory>
#include <KArchiveFile>
#include <KTar>
#include <KZip>
#include <QFile>
#include <QIODevice>
#include <archive.h>
#include <archive_entry.h>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

namespace {
using KiriView::ArchiveDocumentLocation;
using KiriView::archiveEntryPathForUrl;
using KiriView::archiveEntryUrl;
using KiriView::ArchiveError;
using KiriView::ArchiveImageCandidates;
using KiriView::ArchiveImageCandidatesResult;
using KiriView::ArchiveImageData;
using KiriView::ArchiveImageDataResult;
using KiriView::ImageNavigationCandidate;
using KiriView::normalizedArchiveEntryPath;
using LibArchiveReader = std::unique_ptr<archive, int (*)(archive *)>;

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

bool isLibArchiveDocument(const KiriView::ArchiveDocumentLocation &archiveDocument)
{
    return KiriView::archiveStorageBackendForRootScheme(archiveDocument.rootUrl().scheme())
        == KiriView::ArchiveStorageBackend::LibArchive;
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

        const QString candidateName = normalizedArchiveEntryPath(entryPath);
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

QString archiveImageNotFoundError()
{
    return QStringLiteral("Could not find the selected image in the archive.");
}

QString archiveImageReadError()
{
    return QStringLiteral("Could not read the selected archive image.");
}

ArchiveImageCandidatesResult archiveImageCandidatesError(QString errorString)
{
    return ArchiveError { std::move(errorString) };
}

ArchiveImageCandidatesResult archiveImageCandidatesSuccess(
    std::vector<KiriView::ImageNavigationCandidate> candidates)
{
    return ArchiveImageCandidates { std::move(candidates) };
}

ArchiveImageDataResult archiveImageDataError(QString errorString)
{
    return ArchiveError { std::move(errorString) };
}

ArchiveImageDataResult archiveImageDataSuccess(QByteArray data)
{
    return ArchiveImageData { std::move(data) };
}

OpenKArchiveResult openKArchiveDocument(const KiriView::ArchiveDocumentLocation &archiveDocument)
{
    std::unique_ptr<KArchive> archive = createArchive(archiveDocument);
    if (archive == nullptr) {
        return OpenKArchiveResult { {}, fallbackArchiveOpenError(archiveDocument) };
    }

    if (!archive->open(QIODevice::ReadOnly)) {
        const QString errorString = archive->errorString().isEmpty()
            ? fallbackArchiveOpenError(archiveDocument)
            : archive->errorString();
        return OpenKArchiveResult { {}, errorString };
    }

    return OpenKArchiveResult { ScopedKArchive(std::move(archive)), QString() };
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

    *errorString = libArchiveErrorString(reader, archiveImageReadError());
    return false;
}

enum class LibArchiveEntryAction {
    Continue,
    Stop,
};

template <typename EntryHandler>
bool visitLibArchiveEntries(archive *reader, const ArchiveDocumentLocation &archiveDocument,
    QString *errorString, EntryHandler handleEntry)
{
    archive_entry *entry = nullptr;
    while (true) {
        const int status = archive_read_next_header(reader, &entry);
        if (status == ARCHIVE_EOF) {
            return true;
        }
        if (status != ARCHIVE_OK) {
            *errorString = libArchiveErrorString(reader, fallbackArchiveOpenError(archiveDocument));
            return false;
        }

        if (handleEntry(reader, entry) == LibArchiveEntryAction::Stop) {
            return true;
        }

        if (!skipLibArchiveEntry(reader, errorString)) {
            return false;
        }
    }
}

ArchiveImageCandidatesResult loadLibArchiveDocumentImageCandidates(
    const ArchiveDocumentLocation &archiveDocument)
{
    QString errorString;
    LibArchiveReader reader = openLibArchiveReader(archiveDocument, &errorString);
    if (reader == nullptr) {
        return archiveImageCandidatesError(errorString);
    }

    std::vector<ImageNavigationCandidate> candidates;
    const bool visitedEntries = visitLibArchiveEntries(
        reader.get(), archiveDocument, &errorString, [&](archive *, archive_entry *entry) {
            const QString entryPath = normalizedArchiveEntryPath(libArchiveEntryPath(entry));
            if (archive_entry_filetype(entry) == AE_IFREG && !entryPath.isEmpty()
                && KiriView::isSupportedImageFileName(entryPath)) {
                const QUrl url = archiveEntryUrl(archiveDocument, entryPath);
                if (!url.isEmpty()) {
                    candidates.push_back(ImageNavigationCandidate { url, entryPath });
                }
            }

            return LibArchiveEntryAction::Continue;
        });
    if (!visitedEntries) {
        return archiveImageCandidatesError(errorString);
    }

    KiriView::sortImageNavigationCandidates(&candidates);
    return archiveImageCandidatesSuccess(std::move(candidates));
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
            return archiveImageDataError(libArchiveErrorString(reader, archiveImageReadError()));
        }

        data.append(buffer, static_cast<qsizetype>(bytesRead));
    }

    if (archive_entry_size_is_set(entry)) {
        const la_int64_t expectedSize = archive_entry_size(entry);
        if (expectedSize >= 0 && static_cast<qint64>(data.size()) != expectedSize) {
            return archiveImageDataError(archiveImageReadError());
        }
    }

    return archiveImageDataSuccess(std::move(data));
}

ArchiveImageDataResult loadLibArchiveDocumentImageData(
    const ArchiveDocumentLocation &archiveDocument, const QUrl &imageUrl)
{
    const QString entryPath = archiveEntryPathForUrl(archiveDocument, imageUrl);
    if (archiveDocument.isEmpty() || entryPath.isEmpty()) {
        return archiveImageDataError(archiveImageNotFoundError());
    }

    QString errorString;
    LibArchiveReader reader = openLibArchiveReader(archiveDocument, &errorString);
    if (reader == nullptr) {
        return archiveImageDataError(errorString);
    }

    std::optional<ArchiveImageDataResult> result;
    const bool visitedEntries = visitLibArchiveEntries(
        reader.get(), archiveDocument, &errorString, [&](archive *reader, archive_entry *entry) {
            const QString currentPath = normalizedArchiveEntryPath(libArchiveEntryPath(entry));
            if (archive_entry_filetype(entry) == AE_IFREG && currentPath == entryPath) {
                result = readLibArchiveEntryData(reader, entry);
                return LibArchiveEntryAction::Stop;
            }

            return LibArchiveEntryAction::Continue;
        });
    if (!visitedEntries) {
        return archiveImageDataError(errorString);
    }

    if (result.has_value()) {
        return std::move(*result);
    }

    return archiveImageDataError(archiveImageNotFoundError());
}
}

namespace KiriView {
ArchiveImageCandidatesResult loadArchiveDocumentImageCandidates(
    const ArchiveDocumentLocation &archiveDocument)
{
    if (archiveDocument.isEmpty()) {
        return archiveImageCandidatesError(QStringLiteral("Could not open the selected archive."));
    }

    if (isLibArchiveDocument(archiveDocument)) {
        return loadLibArchiveDocumentImageCandidates(archiveDocument);
    }

    OpenKArchiveResult opened = openKArchiveDocument(archiveDocument);
    if (!opened.archive) {
        return archiveImageCandidatesError(opened.errorString);
    }

    std::vector<ImageNavigationCandidate> candidates;
    appendArchiveDirectoryImageCandidates(
        &candidates, opened.archive.directory(), archiveDocument, QString());
    sortImageNavigationCandidates(&candidates);
    return archiveImageCandidatesSuccess(std::move(candidates));
}

ArchiveImageDataResult loadArchiveDocumentImageData(
    const ArchiveDocumentLocation &archiveDocument, const QUrl &imageUrl)
{
    const QString entryPath = archiveEntryPathForUrl(archiveDocument, imageUrl);
    if (archiveDocument.isEmpty() || entryPath.isEmpty()) {
        return archiveImageDataError(archiveImageNotFoundError());
    }

    if (isLibArchiveDocument(archiveDocument)) {
        return loadLibArchiveDocumentImageData(archiveDocument, imageUrl);
    }

    OpenKArchiveResult opened = openKArchiveDocument(archiveDocument);
    if (!opened.archive) {
        return archiveImageDataError(opened.errorString);
    }

    const KArchiveDirectory *directory = opened.archive.directory();
    const KArchiveFile *file = directory == nullptr ? nullptr : directory->file(entryPath);
    if (file == nullptr) {
        return archiveImageDataError(archiveImageNotFoundError());
    }

    std::unique_ptr<QIODevice> device(file->createDevice());
    if (device == nullptr) {
        return archiveImageDataError(archiveImageReadError());
    }

    QByteArray data = device->readAll();
    const qint64 expectedSize = file->size();
    if (expectedSize >= 0 && data.size() != expectedSize) {
        return archiveImageDataError(archiveImageReadError());
    }

    return archiveImageDataSuccess(std::move(data));
}
}
