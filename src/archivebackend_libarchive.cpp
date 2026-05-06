// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivebackend_p.h"

#include "archivepath.h"
#include "imagenavigationmodel.h"

#include <QFile>
#include <archive.h>
#include <archive_entry.h>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
namespace Backend = KiriView::ArchiveBackendDetail;
using LibArchiveReader = std::unique_ptr<archive, int (*)(archive *)>;

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
        *errorString = Backend::fallbackArchiveOpenError(archiveDocument);
        return LibArchiveReader(nullptr, archive_read_free);
    }

    LibArchiveReader reader = makeLibArchiveReader();
    if (reader == nullptr) {
        *errorString = Backend::fallbackArchiveOpenError(archiveDocument);
        return reader;
    }

    if (archive_read_support_filter_all(reader.get()) != ARCHIVE_OK
        || archive_read_support_format_rar(reader.get()) != ARCHIVE_OK
        || archive_read_support_format_rar5(reader.get()) != ARCHIVE_OK) {
        *errorString = libArchiveErrorString(
            reader.get(), Backend::fallbackArchiveOpenError(archiveDocument));
        return LibArchiveReader(nullptr, archive_read_free);
    }

    const QByteArray encodedFilePath = QFile::encodeName(filePath);
    if (archive_read_open_filename(reader.get(), encodedFilePath.constData(), 10240)
        != ARCHIVE_OK) {
        *errorString = libArchiveErrorString(
            reader.get(), Backend::fallbackArchiveOpenError(archiveDocument));
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

    *errorString = libArchiveErrorString(reader, Backend::archiveImageReadError());
    return false;
}

enum class LibArchiveEntryAction {
    Continue,
    Stop,
};

template <typename EntryHandler>
bool visitLibArchiveEntries(archive *reader,
    const KiriView::ArchiveDocumentLocation &archiveDocument, QString *errorString,
    EntryHandler handleEntry)
{
    archive_entry *entry = nullptr;
    while (true) {
        const int status = archive_read_next_header(reader, &entry);
        if (status == ARCHIVE_EOF) {
            return true;
        }
        if (status != ARCHIVE_OK) {
            *errorString
                = libArchiveErrorString(reader, Backend::fallbackArchiveOpenError(archiveDocument));
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

KiriView::ArchiveImageCandidatesResult loadLibArchiveDocumentImageCandidates(
    const KiriView::ArchiveDocumentLocation &archiveDocument)
{
    QString errorString;
    LibArchiveReader reader = openLibArchiveReader(archiveDocument, &errorString);
    if (reader == nullptr) {
        return Backend::archiveErrorResult<KiriView::ArchiveImageCandidatesResult>(errorString);
    }

    std::vector<KiriView::ImageNavigationCandidate> candidates;
    const bool visitedEntries = visitLibArchiveEntries(
        reader.get(), archiveDocument, &errorString, [&](archive *, archive_entry *entry) {
            if (archive_entry_filetype(entry) == AE_IFREG) {
                std::optional<KiriView::ImageNavigationCandidate> candidate
                    = Backend::archiveImageCandidate(archiveDocument, libArchiveEntryPath(entry));
                if (candidate.has_value()) {
                    candidates.push_back(std::move(*candidate));
                }
            }

            return LibArchiveEntryAction::Continue;
        });
    if (!visitedEntries) {
        return Backend::archiveErrorResult<KiriView::ArchiveImageCandidatesResult>(errorString);
    }

    KiriView::sortImageNavigationCandidates(&candidates);
    return Backend::archiveImageCandidatesResult(std::move(candidates));
}

KiriView::ArchiveImageDataResult readLibArchiveEntryData(archive *reader, archive_entry *entry)
{
    QByteArray data;
    char buffer[64 * 1024];
    while (true) {
        const la_ssize_t bytesRead = archive_read_data(reader, buffer, sizeof(buffer));
        if (bytesRead == 0) {
            break;
        }
        if (bytesRead < 0) {
            return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
                libArchiveErrorString(reader, Backend::archiveImageReadError()));
        }

        data.append(buffer, static_cast<qsizetype>(bytesRead));
    }

    if (archive_entry_size_is_set(entry)) {
        const la_int64_t expectedSize = archive_entry_size(entry);
        if (expectedSize >= 0 && static_cast<qint64>(data.size()) != expectedSize) {
            return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
                Backend::archiveImageReadError());
        }
    }

    return Backend::archiveImageDataResult(std::move(data));
}

KiriView::ArchiveImageDataResult loadLibArchiveDocumentImageData(
    const KiriView::ArchiveDocumentLocation &archiveDocument, const QString &entryPath)
{
    QString errorString;
    LibArchiveReader reader = openLibArchiveReader(archiveDocument, &errorString);
    if (reader == nullptr) {
        return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(errorString);
    }

    std::optional<KiriView::ArchiveImageDataResult> result;
    const bool visitedEntries = visitLibArchiveEntries(
        reader.get(), archiveDocument, &errorString, [&](archive *reader, archive_entry *entry) {
            const QString currentPath
                = KiriView::normalizedArchiveEntryPath(libArchiveEntryPath(entry));
            if (archive_entry_filetype(entry) == AE_IFREG && currentPath == entryPath) {
                result = readLibArchiveEntryData(reader, entry);
                return LibArchiveEntryAction::Stop;
            }

            return LibArchiveEntryAction::Continue;
        });
    if (!visitedEntries) {
        return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(errorString);
    }

    if (result.has_value()) {
        return std::move(*result);
    }

    return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
        Backend::archiveImageNotFoundError());
}
}

namespace KiriView::ArchiveBackendDetail {
const ArchiveBackendOperations *libArchiveBackendOperations()
{
    static const ArchiveBackendOperations operations {
        loadLibArchiveDocumentImageCandidates,
        loadLibArchiveDocumentImageData,
    };
    return &operations;
}
}
