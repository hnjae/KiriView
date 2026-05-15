// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivebackend_p.h"

#include "archivepath.h"

#include <QFile>
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <map>
#include <memory>
#include <optional>
#include <unistd.h>
#include <utility>
#include <variant>
#include <vector>

namespace {
namespace Backend = KiriView::ArchiveBackendDetail;
using LibArchiveReader = std::unique_ptr<archive, int (*)(archive *)>;

class ScopedFileDescriptor final
{
public:
    ScopedFileDescriptor() = default;

    explicit ScopedFileDescriptor(int fileDescriptor)
        : m_fileDescriptor(fileDescriptor)
    {
    }

    ~ScopedFileDescriptor() { close(); }

    ScopedFileDescriptor(const ScopedFileDescriptor &) = delete;
    ScopedFileDescriptor &operator=(const ScopedFileDescriptor &) = delete;
    ScopedFileDescriptor(ScopedFileDescriptor &&other) noexcept
        : m_fileDescriptor(std::exchange(other.m_fileDescriptor, -1))
    {
    }
    ScopedFileDescriptor &operator=(ScopedFileDescriptor &&other) noexcept
    {
        if (this == &other) {
            return *this;
        }

        close();
        m_fileDescriptor = std::exchange(other.m_fileDescriptor, -1);
        return *this;
    }

    int get() const { return m_fileDescriptor; }

    explicit operator bool() const { return m_fileDescriptor >= 0; }

private:
    void close()
    {
        if (m_fileDescriptor >= 0) {
            ::close(m_fileDescriptor);
            m_fileDescriptor = -1;
        }
    }

    int m_fileDescriptor = -1;
};

struct OpenArchiveFileResult {
    ScopedFileDescriptor fileDescriptor;
    QString errorString;
};

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

OpenArchiveFileResult openArchiveFileDescriptor(
    const KiriView::ArchiveDocumentLocation &archiveDocument)
{
    const QString filePath = archiveDocument.fileUrl().toLocalFile();
    if (filePath.isEmpty()) {
        return OpenArchiveFileResult { {}, Backend::fallbackArchiveOpenError(archiveDocument) };
    }

#ifdef O_CLOEXEC
    constexpr int openFlags = O_RDONLY | O_CLOEXEC;
#else
    constexpr int openFlags = O_RDONLY;
#endif

    const QByteArray encodedFilePath = QFile::encodeName(filePath);
    const int fileDescriptor = ::open(encodedFilePath.constData(), openFlags);
    if (fileDescriptor < 0) {
        return OpenArchiveFileResult { {}, Backend::fallbackArchiveOpenError(archiveDocument) };
    }

    return OpenArchiveFileResult { ScopedFileDescriptor(fileDescriptor), QString() };
}

bool configureLibArchiveReader(
    archive *reader, const KiriView::ArchiveDocumentLocation &archiveDocument, QString *errorString)
{
    if (archive_read_support_filter_all(reader) != ARCHIVE_OK
        || archive_read_support_format_rar(reader) != ARCHIVE_OK
        || archive_read_support_format_rar5(reader) != ARCHIVE_OK) {
        *errorString
            = libArchiveErrorString(reader, Backend::fallbackArchiveOpenError(archiveDocument));
        return false;
    }

    return true;
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

    if (!configureLibArchiveReader(reader.get(), archiveDocument, errorString)) {
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

LibArchiveReader openLibArchiveReaderOnFd(const KiriView::ArchiveDocumentLocation &archiveDocument,
    int fileDescriptor, QString *errorString)
{
    if (fileDescriptor < 0 || ::lseek(fileDescriptor, 0, SEEK_SET) < 0) {
        *errorString = Backend::fallbackArchiveOpenError(archiveDocument);
        return LibArchiveReader(nullptr, archive_read_free);
    }

    LibArchiveReader reader = makeLibArchiveReader();
    if (reader == nullptr) {
        *errorString = Backend::fallbackArchiveOpenError(archiveDocument);
        return reader;
    }

    if (!configureLibArchiveReader(reader.get(), archiveDocument, errorString)) {
        return LibArchiveReader(nullptr, archive_read_free);
    }

    if (archive_read_open_fd(reader.get(), fileDescriptor, 10240) != ARCHIVE_OK) {
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

class LibArchiveDocumentSession final : public KiriView::ArchiveDocumentSession
{
public:
    static KiriView::ArchiveDocumentSessionOpenResult create(
        const KiriView::ArchiveDocumentLocation &archiveDocument)
    {
        OpenArchiveFileResult opened = openArchiveFileDescriptor(archiveDocument);
        if (!opened.fileDescriptor) {
            return Backend::archiveErrorResult<KiriView::ArchiveDocumentSessionOpenResult>(
                opened.errorString);
        }

        auto session = std::shared_ptr<LibArchiveDocumentSession>(
            new LibArchiveDocumentSession(archiveDocument, std::move(opened.fileDescriptor)));
        QString errorString;
        LibArchiveReader reader
            = openLibArchiveReaderOnFd(archiveDocument, session->m_archiveFile.get(), &errorString);
        if (reader == nullptr) {
            return Backend::archiveErrorResult<KiriView::ArchiveDocumentSessionOpenResult>(
                errorString);
        }

        if (!session->scan(reader.get(), &errorString)) {
            return Backend::archiveErrorResult<KiriView::ArchiveDocumentSessionOpenResult>(
                errorString);
        }

        return KiriView::ArchiveDocumentSessionPtr(std::move(session));
    }

    KiriView::ArchiveImageCandidatesResult loadImageCandidates() override
    {
        return KiriView::ArchiveImageCandidates { m_candidates };
    }

    KiriView::ArchiveImageDataResult loadImageData(const QUrl &imageUrl) override
    {
        const std::optional<QString> entryPath
            = Backend::archiveImageEntryPathForRead(m_archiveDocument, imageUrl);
        if (!entryPath.has_value()) {
            return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
                Backend::archiveImageNotFoundError());
        }

        const auto entryOrder = m_entryOrderByPath.find(*entryPath);
        if (entryOrder == m_entryOrderByPath.cend()) {
            return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
                Backend::archiveImageNotFoundError());
        }

        return readImageDataAtOrder(entryOrder->second);
    }

private:
    LibArchiveDocumentSession(
        KiriView::ArchiveDocumentLocation archiveDocument, ScopedFileDescriptor archiveFile)
        : m_archiveDocument(std::move(archiveDocument))
        , m_archiveFile(std::move(archiveFile))
    {
    }

    bool scan(archive *reader, QString *errorString)
    {
        std::vector<KiriView::ImageNavigationCandidate> candidates;
        std::map<QString, int> entryOrderByPath;
        archive_entry *entry = nullptr;
        int entryOrder = 0;

        while (true) {
            const int status = archive_read_next_header(reader, &entry);
            if (status == ARCHIVE_EOF) {
                KiriView::ArchiveImageCandidatesResult sorted
                    = Backend::archiveImageCandidatesResult(std::move(candidates));
                if (auto *success = std::get_if<KiriView::ArchiveImageCandidates>(&sorted)) {
                    m_candidates = std::move(success->candidates);
                }
                m_entryOrderByPath = std::move(entryOrderByPath);
                return true;
            }
            if (status != ARCHIVE_OK) {
                *errorString = libArchiveErrorString(
                    reader, Backend::fallbackArchiveOpenError(m_archiveDocument));
                return false;
            }

            const int currentEntryOrder = entryOrder;
            ++entryOrder;

            if (archive_entry_filetype(entry) == AE_IFREG) {
                std::optional<KiriView::ImageNavigationCandidate> candidate
                    = Backend::archiveImageCandidate(m_archiveDocument, libArchiveEntryPath(entry));
                if (candidate.has_value()) {
                    entryOrderByPath[candidate->name] = currentEntryOrder;
                    candidates.push_back(std::move(*candidate));
                }
            }

            if (!skipLibArchiveEntry(reader, errorString)) {
                return false;
            }
        }
    }

    KiriView::ArchiveImageDataResult readImageDataAtOrder(int targetEntryOrder)
    {
        QString errorString;
        if (!prepareReaderForEntry(targetEntryOrder, &errorString)) {
            return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(errorString);
        }

        archive_entry *entry = nullptr;
        while (m_nextEntryOrder <= targetEntryOrder) {
            const int currentEntryOrder = m_nextEntryOrder;
            const int status = archive_read_next_header(m_reader.get(), &entry);
            if (status == ARCHIVE_EOF) {
                m_readerExhausted = true;
                return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
                    Backend::archiveImageReadError());
            }
            if (status != ARCHIVE_OK) {
                return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
                    libArchiveErrorString(m_reader.get(), Backend::archiveImageReadError()));
            }

            ++m_nextEntryOrder;
            if (currentEntryOrder == targetEntryOrder) {
                if (archive_entry_filetype(entry) != AE_IFREG) {
                    return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
                        Backend::archiveImageNotFoundError());
                }

                return readLibArchiveEntryData(m_reader.get(), entry);
            }

            if (!skipLibArchiveEntry(m_reader.get(), &errorString)) {
                return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(errorString);
            }
        }

        return Backend::archiveErrorResult<KiriView::ArchiveImageDataResult>(
            Backend::archiveImageReadError());
    }

    bool prepareReaderForEntry(int targetEntryOrder, QString *errorString)
    {
        if (m_reader == nullptr || m_readerExhausted || targetEntryOrder < m_nextEntryOrder) {
            return resetReader(errorString);
        }

        return true;
    }

    bool resetReader(QString *errorString)
    {
        m_reader.reset();
        m_nextEntryOrder = 0;
        m_readerExhausted = false;

        m_reader = openLibArchiveReaderOnFd(m_archiveDocument, m_archiveFile.get(), errorString);
        if (m_reader == nullptr) {
            m_readerExhausted = true;
            return false;
        }

        return true;
    }

    KiriView::ArchiveDocumentLocation m_archiveDocument;
    ScopedFileDescriptor m_archiveFile;
    std::vector<KiriView::ImageNavigationCandidate> m_candidates;
    std::map<QString, int> m_entryOrderByPath;
    LibArchiveReader m_reader { nullptr, archive_read_free };
    int m_nextEntryOrder = 0;
    bool m_readerExhausted = true;
};

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

KiriView::ArchiveDocumentSessionOpenResult openLibArchiveDocumentSession(
    const KiriView::ArchiveDocumentLocation &archiveDocument)
{
    return LibArchiveDocumentSession::create(archiveDocument);
}
}

namespace KiriView::ArchiveBackendDetail {
const ArchiveBackendOperations *libArchiveBackendOperations()
{
    static const ArchiveBackendOperations operations {
        loadLibArchiveDocumentImageCandidates,
        loadLibArchiveDocumentImageData,
        openLibArchiveDocumentSession,
    };
    return &operations;
}
}
