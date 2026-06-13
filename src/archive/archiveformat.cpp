// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archiveformat.h"

#include "bridge/archiveformatconversion.h"
#include "bridge/rustqtconversion.h"
#include "kiriview/src/policy/archiveformat.cxx.h"

#include <QMimeDatabase>
#include <QMimeType>
#include <optional>

namespace {
using ArchiveMatchResolver = std::optional<kiriview::ArchiveOpenMatch> (*)(const QString &);

std::optional<kiriview::ArchiveOpenMatch> archiveMatchForQString(
    const QString &value, kiriview::RustArchiveOpenMatch (*rustFunction)(rust::Str))
{
    return kiriview::archiveOpenMatchFromBridge(
        kiriview::Bridge::rustResultForQString(value, rustFunction));
}

QString schemeString(const std::optional<kiriview::ArchiveOpenMatch> &match)
{
    return match.has_value() ? match->scheme : QString();
}

std::optional<kiriview::ArchiveOpenMatch> archiveMatchForUrl(const QUrl &url,
    QMimeDatabase::MatchMode mimeMatchMode, ArchiveMatchResolver matchForFileName,
    ArchiveMatchResolver matchForMimeTypeName)
{
    if (!url.isLocalFile()) {
        return std::nullopt;
    }

    std::optional<kiriview::ArchiveOpenMatch> extensionMatch = matchForFileName(url.fileName());
    if (extensionMatch.has_value()) {
        return extensionMatch;
    }

    const QMimeType mimeType = QMimeDatabase().mimeTypeForFile(url.toLocalFile(), mimeMatchMode);
    return matchForMimeTypeName(mimeType.name());
}

}

namespace kiriview {
bool isSupportedArchiveRootScheme(const QString &scheme)
{
    return Bridge::rustResultForQString(scheme, rustIsSupportedArchiveRootScheme);
}

ArchiveStorageBackend archiveStorageBackendForRootScheme(const QString &scheme)
{
    return archiveStorageBackendFromBridge(
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
        kiriview::comicBookArchiveMatchForFileName, kiriview::comicBookArchiveMatchForMimeTypeName);
}

std::optional<ArchiveOpenMatch> directArchiveOpenMatchForUrl(const QUrl &url)
{
    return archiveMatchForUrl(url, QMimeDatabase::MatchDefault,
        kiriview::directArchiveOpenMatchForFileName,
        kiriview::directArchiveOpenMatchForMimeTypeName);
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
