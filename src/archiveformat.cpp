// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archiveformat.h"

#include <cstddef>
#include <iterator>

namespace {
struct ArchiveMimeTypes {
    const char *const *values = nullptr;
    std::size_t size = 0;
};

struct ArchiveOpenProfile {
    const char *extension;
    ArchiveMimeTypes mimeTypes;
    const char *marker;
};

struct ArchiveFormat {
    const char *scheme;
    ArchiveOpenProfile comicBook;
    ArchiveOpenProfile directArchive;
    KiriView::ArchiveStorageBackend backend;
};

enum class ArchiveOpenMode {
    ComicBook,
    DirectArchive,
};

template <std::size_t Size>
constexpr ArchiveMimeTypes archiveMimeTypes(const char *const (&values)[Size])
{
    return ArchiveMimeTypes { values, Size };
}

const char *const zipComicBookMimeTypes[] = { "application/vnd.comicbook+zip" };
const char *const zipDirectArchiveMimeTypes[] = { "application/zip" };
const char *const tarComicBookMimeTypes[] = { "application/x-cbt" };
const char *const tarDirectArchiveMimeTypes[] = { "application/x-tar" };
const char *const sevenZipComicBookMimeTypes[] = { "application/x-cb7" };
const char *const sevenZipDirectArchiveMimeTypes[] = { "application/x-7z-compressed" };
const char *const rarComicBookMimeTypes[]
    = { "application/vnd.comicbook-rar", "application/x-cbr" };
const char *const rarDirectArchiveMimeTypes[]
    = { "application/vnd.rar", "application/x-rar", "application/x-rar-compressed" };

const ArchiveFormat archiveFormats[] = {
    { "zip", { "cbz", archiveMimeTypes(zipComicBookMimeTypes), ".cbz/" },
        { "zip", archiveMimeTypes(zipDirectArchiveMimeTypes), ".zip/" },
        KiriView::ArchiveStorageBackend::KZip },
    { "tar", { "cbt", archiveMimeTypes(tarComicBookMimeTypes), ".cbt/" },
        { "tar", archiveMimeTypes(tarDirectArchiveMimeTypes), ".tar/" },
        KiriView::ArchiveStorageBackend::KTar },
    { "sevenz", { "cb7", archiveMimeTypes(sevenZipComicBookMimeTypes), ".cb7/" },
        { "7z", archiveMimeTypes(sevenZipDirectArchiveMimeTypes), ".7z/" },
        KiriView::ArchiveStorageBackend::K7Zip },
    { "rar", { "cbr", archiveMimeTypes(rarComicBookMimeTypes), ".cbr/" },
        { "rar", archiveMimeTypes(rarDirectArchiveMimeTypes), ".rar/" },
        KiriView::ArchiveStorageBackend::LibArchive },
};

const ArchiveFormat *archiveFormatForScheme(const QString &scheme)
{
    for (const ArchiveFormat &format : archiveFormats) {
        if (scheme == QString::fromLatin1(format.scheme)) {
            return &format;
        }
    }

    return nullptr;
}

QString extensionForFileName(const QString &fileName)
{
    const qsizetype dotIndex = fileName.lastIndexOf(QLatin1Char('.'));
    if (dotIndex <= 0 || dotIndex == fileName.size() - 1) {
        return {};
    }

    return fileName.mid(dotIndex + 1).toCaseFolded();
}

template <typename Predicate>
bool acceptedProfileMatches(const ArchiveFormat &format, ArchiveOpenMode mode, Predicate predicate)
{
    if (predicate(format.comicBook)) {
        return true;
    }

    return mode == ArchiveOpenMode::DirectArchive && predicate(format.directArchive);
}

bool archiveFormatAcceptsExtension(
    const ArchiveFormat &format, const QString &extension, ArchiveOpenMode mode)
{
    return acceptedProfileMatches(format, mode, [&extension](const ArchiveOpenProfile &profile) {
        return extension == QString::fromLatin1(profile.extension);
    });
}

const ArchiveFormat *archiveFormatForExtension(const QString &extension, ArchiveOpenMode mode)
{
    for (const ArchiveFormat &format : archiveFormats) {
        if (archiveFormatAcceptsExtension(format, extension, mode)) {
            return &format;
        }
    }

    return nullptr;
}

bool mimeTypesContain(const ArchiveMimeTypes &mimeTypes, const QString &mimeTypeName)
{
    for (std::size_t index = 0; index < mimeTypes.size; ++index) {
        if (mimeTypeName == QString::fromLatin1(mimeTypes.values[index])) {
            return true;
        }
    }

    return false;
}

bool archiveFormatAcceptsMimeTypeName(
    const ArchiveFormat &format, const QString &mimeTypeName, ArchiveOpenMode mode)
{
    return acceptedProfileMatches(format, mode, [&mimeTypeName](const ArchiveOpenProfile &profile) {
        return mimeTypesContain(profile.mimeTypes, mimeTypeName);
    });
}

const ArchiveFormat *archiveFormatForMimeTypeName(const QString &mimeTypeName, ArchiveOpenMode mode)
{
    for (const ArchiveFormat &format : archiveFormats) {
        if (archiveFormatAcceptsMimeTypeName(format, mimeTypeName, mode)) {
            return &format;
        }
    }

    return nullptr;
}

bool storageBackendUsesKioFuse(KiriView::ArchiveStorageBackend backend)
{
    switch (backend) {
    case KiriView::ArchiveStorageBackend::KZip:
    case KiriView::ArchiveStorageBackend::KTar:
    case KiriView::ArchiveStorageBackend::K7Zip:
        return true;
    case KiriView::ArchiveStorageBackend::LibArchive:
    case KiriView::ArchiveStorageBackend::None:
        return false;
    }

    return false;
}

QString schemeString(const ArchiveFormat *format)
{
    return format == nullptr ? QString() : QString::fromLatin1(format->scheme);
}

QString markerString(const char *marker) { return QString::fromLatin1(marker); }
}

namespace KiriView {
bool isSupportedArchiveRootScheme(const QString &scheme)
{
    return archiveFormatForScheme(scheme) != nullptr;
}

ArchiveStorageBackend archiveStorageBackendForRootScheme(const QString &scheme)
{
    const ArchiveFormat *format = archiveFormatForScheme(scheme);
    return format == nullptr ? ArchiveStorageBackend::None : format->backend;
}

bool archiveRootSchemeUsesKioFuse(const QString &scheme)
{
    return storageBackendUsesKioFuse(archiveStorageBackendForRootScheme(scheme));
}

QStringList supportedComicBookArchiveExtensions()
{
    QStringList extensions;
    extensions.reserve(static_cast<qsizetype>(std::size(archiveFormats)));
    for (const ArchiveFormat &format : archiveFormats) {
        extensions.append(QString::fromLatin1(format.comicBook.extension));
    }

    return extensions;
}

QString comicBookArchiveKioSchemeForFileName(const QString &fileName)
{
    return schemeString(
        archiveFormatForExtension(extensionForFileName(fileName), ArchiveOpenMode::ComicBook));
}

QString directArchiveOpenKioSchemeForFileName(const QString &fileName)
{
    return schemeString(
        archiveFormatForExtension(extensionForFileName(fileName), ArchiveOpenMode::DirectArchive));
}

QString comicBookArchiveKioSchemeForMimeTypeName(const QString &mimeTypeName)
{
    return schemeString(archiveFormatForMimeTypeName(mimeTypeName, ArchiveOpenMode::ComicBook));
}

QString directArchiveOpenKioSchemeForMimeTypeName(const QString &mimeTypeName)
{
    return schemeString(archiveFormatForMimeTypeName(mimeTypeName, ArchiveOpenMode::DirectArchive));
}

QString comicBookArchiveMarkerForRootScheme(const QString &scheme)
{
    const ArchiveFormat *format = archiveFormatForScheme(scheme);
    return format == nullptr ? QString() : markerString(format->comicBook.marker);
}

QStringList directArchiveOpenMarkersForRootScheme(const QString &scheme)
{
    const ArchiveFormat *format = archiveFormatForScheme(scheme);
    if (format == nullptr) {
        return {};
    }

    return {
        markerString(format->comicBook.marker),
        markerString(format->directArchive.marker),
    };
}
}
