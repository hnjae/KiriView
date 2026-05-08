// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archiveformat.h"

#include <cstddef>
#include <iterator>
#include <optional>

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
std::optional<KiriView::ArchiveOpenMatch> acceptedProfileMatch(
    const ArchiveFormat &format, ArchiveProfileSet profileSet, Predicate predicate)
{
    if (predicate(format.comicBook)) {
        return KiriView::ArchiveOpenMatch { QString::fromLatin1(format.scheme),
            KiriView::ArchiveOpenMatchKind::ComicBook };
    }

    if (profileSet == ArchiveProfileSet::DirectlyOpenable && predicate(format.directArchive)) {
        return KiriView::ArchiveOpenMatch { QString::fromLatin1(format.scheme),
            KiriView::ArchiveOpenMatchKind::GeneralArchive };
    }

    return std::nullopt;
}

template <typename Predicate>
std::optional<KiriView::ArchiveOpenMatch> archiveMatch(
    ArchiveProfileSet profileSet, Predicate predicate)
{
    for (const ArchiveFormat &format : archiveFormats) {
        std::optional<KiriView::ArchiveOpenMatch> match
            = acceptedProfileMatch(format, profileSet, predicate);
        if (match.has_value()) {
            return match;
        }
    }

    return std::nullopt;
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

std::optional<KiriView::ArchiveOpenMatch> archiveMatchForExtension(
    const QString &extension, ArchiveProfileSet profileSet)
{
    return archiveMatch(profileSet, [&extension](const ArchiveOpenProfile &profile) {
        return extension == QString::fromLatin1(profile.extension);
    });
}

std::optional<KiriView::ArchiveOpenMatch> archiveMatchForMimeTypeName(
    const QString &mimeTypeName, ArchiveProfileSet profileSet)
{
    return archiveMatch(profileSet, [&mimeTypeName](const ArchiveOpenProfile &profile) {
        return mimeTypesContain(profile.mimeTypes, mimeTypeName);
    });
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

QString schemeString(const std::optional<KiriView::ArchiveOpenMatch> &match)
{
    return match.has_value() ? match->scheme : QString();
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

std::optional<ArchiveOpenMatch> comicBookArchiveMatchForFileName(const QString &fileName)
{
    return archiveMatchForExtension(
        extensionForFileName(fileName), ArchiveProfileSet::ComicBookOnly);
}

std::optional<ArchiveOpenMatch> directArchiveOpenMatchForFileName(const QString &fileName)
{
    return archiveMatchForExtension(
        extensionForFileName(fileName), ArchiveProfileSet::DirectlyOpenable);
}

std::optional<ArchiveOpenMatch> comicBookArchiveMatchForMimeTypeName(const QString &mimeTypeName)
{
    return archiveMatchForMimeTypeName(mimeTypeName, ArchiveProfileSet::ComicBookOnly);
}

std::optional<ArchiveOpenMatch> directArchiveOpenMatchForMimeTypeName(const QString &mimeTypeName)
{
    return archiveMatchForMimeTypeName(mimeTypeName, ArchiveProfileSet::DirectlyOpenable);
}

QString comicBookArchiveKioSchemeForFileName(const QString &fileName)
{
    return schemeString(comicBookArchiveMatchForFileName(fileName));
}

QString directArchiveOpenKioSchemeForFileName(const QString &fileName)
{
    return schemeString(directArchiveOpenMatchForFileName(fileName));
}

QString comicBookArchiveKioSchemeForMimeTypeName(const QString &mimeTypeName)
{
    return schemeString(comicBookArchiveMatchForMimeTypeName(mimeTypeName));
}

QString directArchiveOpenKioSchemeForMimeTypeName(const QString &mimeTypeName)
{
    return schemeString(directArchiveOpenMatchForMimeTypeName(mimeTypeName));
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
