// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archiveformat.h"

namespace {
struct ArchiveRootFormat {
    const char *scheme;
    const char *comicBookMarker;
    const char *generalArchiveMarker;
    KiriView::ArchiveStorageBackend backend;
};

const ArchiveRootFormat archiveRootFormats[] = {
    { "zip", ".cbz/", ".zip/", KiriView::ArchiveStorageBackend::KZip },
    { "tar", ".cbt/", ".tar/", KiriView::ArchiveStorageBackend::KTar },
    { "sevenz", ".cb7/", ".7z/", KiriView::ArchiveStorageBackend::K7Zip },
    { "rar", ".cbr/", ".rar/", KiriView::ArchiveStorageBackend::LibArchive },
};

const ArchiveRootFormat *archiveRootFormatForScheme(const QString &scheme)
{
    for (const ArchiveRootFormat &format : archiveRootFormats) {
        if (scheme == QString::fromLatin1(format.scheme)) {
            return &format;
        }
    }

    return nullptr;
}

QString markerString(const char *marker) { return QString::fromLatin1(marker); }
}

namespace KiriView {
bool isSupportedArchiveRootScheme(const QString &scheme)
{
    return archiveRootFormatForScheme(scheme) != nullptr;
}

ArchiveStorageBackend archiveStorageBackendForRootScheme(const QString &scheme)
{
    const ArchiveRootFormat *format = archiveRootFormatForScheme(scheme);
    return format == nullptr ? ArchiveStorageBackend::None : format->backend;
}

QString comicBookArchiveMarkerForRootScheme(const QString &scheme)
{
    const ArchiveRootFormat *format = archiveRootFormatForScheme(scheme);
    return format == nullptr ? QString() : markerString(format->comicBookMarker);
}

QStringList directArchiveOpenMarkersForRootScheme(const QString &scheme)
{
    const ArchiveRootFormat *format = archiveRootFormatForScheme(scheme);
    if (format == nullptr) {
        return {};
    }

    return {
        markerString(format->comicBookMarker),
        markerString(format->generalArchiveMarker),
    };
}
}
