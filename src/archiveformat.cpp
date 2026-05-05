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

struct ArchiveFormat {
    const char *scheme;
    const char *comicBookExtension;
    const char *directArchiveExtension;
    ArchiveMimeTypes comicBookMimeTypes;
    ArchiveMimeTypes directArchiveMimeTypes;
    const char *comicBookMarker;
    const char *generalArchiveMarker;
    KiriView::ArchiveStorageBackend backend;
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
    { "zip", "cbz", "zip", archiveMimeTypes(zipComicBookMimeTypes),
        archiveMimeTypes(zipDirectArchiveMimeTypes), ".cbz/", ".zip/",
        KiriView::ArchiveStorageBackend::KZip },
    { "tar", "cbt", "tar", archiveMimeTypes(tarComicBookMimeTypes),
        archiveMimeTypes(tarDirectArchiveMimeTypes), ".cbt/", ".tar/",
        KiriView::ArchiveStorageBackend::KTar },
    { "sevenz", "cb7", "7z", archiveMimeTypes(sevenZipComicBookMimeTypes),
        archiveMimeTypes(sevenZipDirectArchiveMimeTypes), ".cb7/", ".7z/",
        KiriView::ArchiveStorageBackend::K7Zip },
    { "rar", "cbr", "rar", archiveMimeTypes(rarComicBookMimeTypes),
        archiveMimeTypes(rarDirectArchiveMimeTypes), ".cbr/", ".rar/",
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

const ArchiveFormat *archiveFormatForComicBookExtension(const QString &extension)
{
    for (const ArchiveFormat &format : archiveFormats) {
        if (extension == QString::fromLatin1(format.comicBookExtension)) {
            return &format;
        }
    }

    return nullptr;
}

const ArchiveFormat *archiveFormatForDirectArchiveExtension(const QString &extension)
{
    if (const ArchiveFormat *format = archiveFormatForComicBookExtension(extension)) {
        return format;
    }

    for (const ArchiveFormat &format : archiveFormats) {
        if (extension == QString::fromLatin1(format.directArchiveExtension)) {
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

const ArchiveFormat *archiveFormatForComicBookMimeTypeName(const QString &mimeTypeName)
{
    for (const ArchiveFormat &format : archiveFormats) {
        if (mimeTypesContain(format.comicBookMimeTypes, mimeTypeName)) {
            return &format;
        }
    }

    return nullptr;
}

const ArchiveFormat *archiveFormatForDirectArchiveMimeTypeName(const QString &mimeTypeName)
{
    if (const ArchiveFormat *format = archiveFormatForComicBookMimeTypeName(mimeTypeName)) {
        return format;
    }

    for (const ArchiveFormat &format : archiveFormats) {
        if (mimeTypesContain(format.directArchiveMimeTypes, mimeTypeName)) {
            return &format;
        }
    }

    return nullptr;
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

QStringList supportedComicBookArchiveExtensions()
{
    QStringList extensions;
    extensions.reserve(static_cast<qsizetype>(std::size(archiveFormats)));
    for (const ArchiveFormat &format : archiveFormats) {
        extensions.append(QString::fromLatin1(format.comicBookExtension));
    }

    return extensions;
}

QString comicBookArchiveKioSchemeForFileName(const QString &fileName)
{
    return schemeString(archiveFormatForComicBookExtension(extensionForFileName(fileName)));
}

QString directArchiveOpenKioSchemeForFileName(const QString &fileName)
{
    return schemeString(archiveFormatForDirectArchiveExtension(extensionForFileName(fileName)));
}

QString comicBookArchiveKioSchemeForMimeTypeName(const QString &mimeTypeName)
{
    return schemeString(archiveFormatForComicBookMimeTypeName(mimeTypeName));
}

QString directArchiveOpenKioSchemeForMimeTypeName(const QString &mimeTypeName)
{
    return schemeString(archiveFormatForDirectArchiveMimeTypeName(mimeTypeName));
}

QString comicBookArchiveMarkerForRootScheme(const QString &scheme)
{
    const ArchiveFormat *format = archiveFormatForScheme(scheme);
    return format == nullptr ? QString() : markerString(format->comicBookMarker);
}

QStringList directArchiveOpenMarkersForRootScheme(const QString &scheme)
{
    const ArchiveFormat *format = archiveFormatForScheme(scheme);
    if (format == nullptr) {
        return {};
    }

    return {
        markerString(format->comicBookMarker),
        markerString(format->generalArchiveMarker),
    };
}
}
