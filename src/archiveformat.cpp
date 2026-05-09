// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archiveformat.h"

#include "kiriview/src/imageformatregistry.cxx.h"
#include "rustqtconversion.h"

#include <optional>

namespace {
KiriView::ArchiveStorageBackend archiveStorageBackendFromRust(
    KiriView::RustArchiveStorageBackend backend)
{
    switch (backend) {
    case KiriView::RustArchiveStorageBackend::KZip:
        return KiriView::ArchiveStorageBackend::KZip;
    case KiriView::RustArchiveStorageBackend::KTar:
        return KiriView::ArchiveStorageBackend::KTar;
    case KiriView::RustArchiveStorageBackend::K7Zip:
        return KiriView::ArchiveStorageBackend::K7Zip;
    case KiriView::RustArchiveStorageBackend::LibArchive:
        return KiriView::ArchiveStorageBackend::LibArchive;
    case KiriView::RustArchiveStorageBackend::None:
        return KiriView::ArchiveStorageBackend::None;
    }

    return KiriView::ArchiveStorageBackend::None;
}

KiriView::ArchiveOpenMatchKind archiveOpenMatchKindFromRust(KiriView::RustArchiveOpenMatchKind kind)
{
    switch (kind) {
    case KiriView::RustArchiveOpenMatchKind::ComicBook:
        return KiriView::ArchiveOpenMatchKind::ComicBook;
    case KiriView::RustArchiveOpenMatchKind::GeneralArchive:
        return KiriView::ArchiveOpenMatchKind::GeneralArchive;
    }

    return KiriView::ArchiveOpenMatchKind::GeneralArchive;
}

std::optional<KiriView::ArchiveOpenMatch> archiveOpenMatchFromRust(
    const KiriView::RustArchiveOpenMatch &match)
{
    if (!match.found) {
        return std::nullopt;
    }

    return KiriView::ArchiveOpenMatch {
        KiriView::Bridge::qtString(match.scheme),
        archiveOpenMatchKindFromRust(match.kind),
    };
}

std::optional<KiriView::ArchiveOpenMatch> archiveMatchForQString(
    const QString &value, KiriView::RustArchiveOpenMatch (*rustFunction)(rust::Str))
{
    return archiveOpenMatchFromRust(KiriView::Bridge::rustResultForQString(value, rustFunction));
}

QString schemeString(const std::optional<KiriView::ArchiveOpenMatch> &match)
{
    return match.has_value() ? match->scheme : QString();
}
}

namespace KiriView {
bool isSupportedArchiveRootScheme(const QString &scheme)
{
    return Bridge::rustResultForQString(scheme, rustIsSupportedArchiveRootScheme);
}

ArchiveStorageBackend archiveStorageBackendForRootScheme(const QString &scheme)
{
    return archiveStorageBackendFromRust(
        Bridge::rustResultForQString(scheme, rustArchiveStorageBackendForRootScheme));
}

bool archiveRootSchemeUsesKioFuse(const QString &scheme)
{
    return Bridge::rustResultForQString(scheme, rustArchiveRootSchemeUsesKioFuse);
}

QStringList supportedComicBookArchiveExtensions()
{
    return Bridge::qtStringList(rustSupportedComicBookArchiveExtensions());
}

QStringList supportedComicBookArchiveMimeTypes()
{
    return Bridge::qtStringList(rustSupportedComicBookArchiveMimeTypes());
}

std::optional<ArchiveOpenMatch> comicBookArchiveMatchForFileName(const QString &fileName)
{
    return archiveMatchForQString(fileName, rustComicBookArchiveMatchForFileName);
}

std::optional<ArchiveOpenMatch> directArchiveOpenMatchForFileName(const QString &fileName)
{
    return archiveMatchForQString(fileName, rustDirectArchiveOpenMatchForFileName);
}

std::optional<ArchiveOpenMatch> comicBookArchiveMatchForMimeTypeName(const QString &mimeTypeName)
{
    return archiveMatchForQString(mimeTypeName, rustComicBookArchiveMatchForMimeTypeName);
}

std::optional<ArchiveOpenMatch> directArchiveOpenMatchForMimeTypeName(const QString &mimeTypeName)
{
    return archiveMatchForQString(mimeTypeName, rustDirectArchiveOpenMatchForMimeTypeName);
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
    return Bridge::qtString(
        Bridge::rustResultForQString(scheme, rustComicBookArchiveMarkerForRootScheme));
}

QStringList directArchiveOpenMarkersForRootScheme(const QString &scheme)
{
    return Bridge::qtStringList(
        Bridge::rustResultForQString(scheme, rustDirectArchiveOpenMarkersForRootScheme));
}
}
