// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archiveformat.h"

#include "kiriview/src/archiveformat.cxx.h"
#include "rustqtconversion.h"

#include <QMimeDatabase>
#include <QMimeType>
#include <optional>

namespace {
using ArchiveMatchResolver = std::optional<KiriView::ArchiveOpenMatch> (*)(const QString &);

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

std::optional<KiriView::ArchiveOpenMatch> archiveMatchForUrl(const QUrl &url,
    QMimeDatabase::MatchMode mimeMatchMode, ArchiveMatchResolver matchForFileName,
    ArchiveMatchResolver matchForMimeTypeName)
{
    if (!url.isLocalFile()) {
        return std::nullopt;
    }

    std::optional<KiriView::ArchiveOpenMatch> extensionMatch = matchForFileName(url.fileName());
    if (extensionMatch.has_value()) {
        return extensionMatch;
    }

    const QMimeType mimeType = QMimeDatabase().mimeTypeForFile(url.toLocalFile(), mimeMatchMode);
    return matchForMimeTypeName(mimeType.name());
}

QString archiveKioSchemeForUrl(const QUrl &url, QMimeDatabase::MatchMode mimeMatchMode,
    ArchiveMatchResolver matchForFileName, ArchiveMatchResolver matchForMimeTypeName)
{
    return schemeString(
        archiveMatchForUrl(url, mimeMatchMode, matchForFileName, matchForMimeTypeName));
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

bool isComicBookArchiveFileName(const QString &name)
{
    return !comicBookArchiveKioSchemeForFileName(name).isEmpty();
}

bool isComicBookArchiveUrl(const QUrl &url)
{
    return !comicBookArchiveKioSchemeForUrl(url).isEmpty();
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

std::optional<ArchiveOpenMatch> comicBookArchiveMatchForUrl(const QUrl &url)
{
    return archiveMatchForUrl(url, QMimeDatabase::MatchExtension,
        KiriView::comicBookArchiveMatchForFileName, KiriView::comicBookArchiveMatchForMimeTypeName);
}

std::optional<ArchiveOpenMatch> directArchiveOpenMatchForUrl(const QUrl &url)
{
    return archiveMatchForUrl(url, QMimeDatabase::MatchDefault,
        KiriView::directArchiveOpenMatchForFileName,
        KiriView::directArchiveOpenMatchForMimeTypeName);
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

QString comicBookArchiveKioSchemeForUrl(const QUrl &url)
{
    return schemeString(comicBookArchiveMatchForUrl(url));
}

QString directArchiveOpenKioSchemeForUrl(const QUrl &url)
{
    return schemeString(directArchiveOpenMatchForUrl(url));
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
