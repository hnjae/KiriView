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
};

struct ArchiveFormat {
    const char *scheme;
    ArchiveOpenProfile comicBook;
    ArchiveOpenProfile directArchive;
    KiriView::ArchiveStorageBackend backend;
};

enum class ArchiveProfileSet {
    ComicBookOnly,
    DirectlyOpenable,
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
    { "zip", { "cbz", archiveMimeTypes(zipComicBookMimeTypes) },
        { "zip", archiveMimeTypes(zipDirectArchiveMimeTypes) },
        KiriView::ArchiveStorageBackend::KZip },
    { "tar", { "cbt", archiveMimeTypes(tarComicBookMimeTypes) },
        { "tar", archiveMimeTypes(tarDirectArchiveMimeTypes) },
        KiriView::ArchiveStorageBackend::KTar },
    { "sevenz", { "cb7", archiveMimeTypes(sevenZipComicBookMimeTypes) },
        { "7z", archiveMimeTypes(sevenZipDirectArchiveMimeTypes) },
        KiriView::ArchiveStorageBackend::K7Zip },
    { "rar", { "cbr", archiveMimeTypes(rarComicBookMimeTypes) },
        { "rar", archiveMimeTypes(rarDirectArchiveMimeTypes) },
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
bool acceptedProfileMatches(
    const ArchiveFormat &format, ArchiveProfileSet profileSet, Predicate predicate)
{
    if (predicate(format.comicBook)) {
        return true;
    }

    return profileSet == ArchiveProfileSet::DirectlyOpenable && predicate(format.directArchive);
}

bool archiveFormatAcceptsExtension(
    const ArchiveFormat &format, const QString &extension, ArchiveProfileSet profileSet)
{
    return acceptedProfileMatches(
        format, profileSet, [&extension](const ArchiveOpenProfile &profile) {
            return extension == QString::fromLatin1(profile.extension);
        });
}

const ArchiveFormat *archiveFormatForExtension(
    const QString &extension, ArchiveProfileSet profileSet)
{
    for (const ArchiveFormat &format : archiveFormats) {
        if (archiveFormatAcceptsExtension(format, extension, profileSet)) {
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
    const ArchiveFormat &format, const QString &mimeTypeName, ArchiveProfileSet profileSet)
{
    return acceptedProfileMatches(
        format, profileSet, [&mimeTypeName](const ArchiveOpenProfile &profile) {
            return mimeTypesContain(profile.mimeTypes, mimeTypeName);
        });
}

const ArchiveFormat *archiveFormatForMimeTypeName(
    const QString &mimeTypeName, ArchiveProfileSet profileSet)
{
    for (const ArchiveFormat &format : archiveFormats) {
        if (archiveFormatAcceptsMimeTypeName(format, mimeTypeName, profileSet)) {
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

QString markerString(const ArchiveOpenProfile &profile)
{
    return QStringLiteral(".") + QString::fromLatin1(profile.extension) + QLatin1Char('/');
}

void appendMimeTypes(QStringList *mimeTypeNames, const ArchiveMimeTypes &mimeTypes)
{
    for (std::size_t index = 0; index < mimeTypes.size; ++index) {
        mimeTypeNames->append(QString::fromLatin1(mimeTypes.values[index]));
    }
}
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

QStringList supportedComicBookArchiveMimeTypes()
{
    QStringList mimeTypes;
    for (const ArchiveFormat &format : archiveFormats) {
        appendMimeTypes(&mimeTypes, format.comicBook.mimeTypes);
    }

    mimeTypes.sort();
    mimeTypes.removeDuplicates();
    return mimeTypes;
}

QString comicBookArchiveKioSchemeForFileName(const QString &fileName)
{
    return schemeString(archiveFormatForExtension(
        extensionForFileName(fileName), ArchiveProfileSet::ComicBookOnly));
}

QString directArchiveOpenKioSchemeForFileName(const QString &fileName)
{
    return schemeString(archiveFormatForExtension(
        extensionForFileName(fileName), ArchiveProfileSet::DirectlyOpenable));
}

QString comicBookArchiveKioSchemeForMimeTypeName(const QString &mimeTypeName)
{
    return schemeString(
        archiveFormatForMimeTypeName(mimeTypeName, ArchiveProfileSet::ComicBookOnly));
}

QString directArchiveOpenKioSchemeForMimeTypeName(const QString &mimeTypeName)
{
    return schemeString(
        archiveFormatForMimeTypeName(mimeTypeName, ArchiveProfileSet::DirectlyOpenable));
}

QString comicBookArchiveMarkerForRootScheme(const QString &scheme)
{
    const ArchiveFormat *format = archiveFormatForScheme(scheme);
    return format == nullptr ? QString() : markerString(format->comicBook);
}

QStringList directArchiveOpenMarkersForRootScheme(const QString &scheme)
{
    const ArchiveFormat *format = archiveFormatForScheme(scheme);
    if (format == nullptr) {
        return {};
    }

    return {
        markerString(format->comicBook),
        markerString(format->directArchive),
    };
}
}
