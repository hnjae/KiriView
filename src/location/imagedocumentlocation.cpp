// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "location/imagedocumentlocation.h"

#include "archive/archiveformat.h"
#include "bridge/rustqtconversion.h"
#include "kiriview/src/policy/archivepath.cxx.h"
#include "location/imageurl.h"

#include <QByteArray>
#include <QFileInfo>
#include <optional>

namespace {
struct ArchiveCollectionRoot {
    QUrl rootUrl;
    kiriview::OpenedCollectionScopeKind kind = kiriview::OpenedCollectionScopeKind::GeneralArchive;
};

struct UrlParts {
    QByteArray scheme;
    QByteArray path;
    bool empty = true;
};

UrlParts urlParts(const QUrl &url)
{
    return UrlParts { url.scheme().toUtf8(), url.path().toUtf8(), url.isEmpty() };
}

std::optional<QUrl> archiveRootUrlForLocalArchive(const QUrl &url, const QString &archiveScheme)
{
    const QByteArray archiveSchemeBytes = archiveScheme.toUtf8();
    const QByteArray localPathBytes = url.toLocalFile().toUtf8();
    const kiriview::RustArchiveRootPath rootPath = kiriview::rustArchiveRootPathForLocalArchive(
        url.isLocalFile(), kiriview::Bridge::rustStr(archiveSchemeBytes),
        kiriview::Bridge::rustStr(localPathBytes));
    if (!rootPath.found) {
        return std::nullopt;
    }

    QUrl archiveRootUrl;
    archiveRootUrl.setScheme(archiveScheme);
    archiveRootUrl.setPath(kiriview::Bridge::qtString(rootPath.path));
    if (!archiveRootUrl.isValid() || archiveRootUrl.path().isEmpty()) {
        return std::nullopt;
    }

    return archiveRootUrl;
}

kiriview::OpenedCollectionScopeKind archiveCollectionKindForMatch(
    const kiriview::ArchiveOpenMatch &match)
{
    switch (match.kind) {
    case kiriview::ArchiveOpenMatchKind::ComicBook:
        return kiriview::OpenedCollectionScopeKind::ComicBookArchive;
    case kiriview::ArchiveOpenMatchKind::GeneralArchive:
        return kiriview::OpenedCollectionScopeKind::GeneralArchive;
    }

    return kiriview::OpenedCollectionScopeKind::GeneralArchive;
}

std::optional<ArchiveCollectionRoot> archiveCollectionRootForLocalArchive(
    const QUrl &url, std::optional<kiriview::ArchiveOpenMatch> match)
{
    if (!match.has_value()) {
        return std::nullopt;
    }

    const std::optional<QUrl> rootUrl = archiveRootUrlForLocalArchive(url, match->scheme);
    if (!rootUrl.has_value()) {
        return std::nullopt;
    }

    return ArchiveCollectionRoot { *rootUrl, archiveCollectionKindForMatch(*match) };
}

std::optional<ArchiveCollectionRoot> directArchiveCollectionRootForLocalArchive(const QUrl &url)
{
    return archiveCollectionRootForLocalArchive(url, kiriview::directArchiveOpenMatchForUrl(url));
}

std::optional<kiriview::OpenedCollectionScopeLocation>
directoryOpenedCollectionScopeLocationForLocalUrl(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return std::nullopt;
    }

    const QUrl fileUrl = kiriview::normalizedFileContainerUrl(url);
    const QString localPath = fileUrl.toLocalFile();
    if (localPath.isEmpty() || !QFileInfo(localPath).isDir()) {
        return std::nullopt;
    }

    return kiriview::OpenedCollectionScopeLocation::fromUrls(fileUrl,
        kiriview::normalizedDirectoryContainerUrl(fileUrl),
        kiriview::OpenedCollectionScopeKind::Directory);
}

std::optional<QUrl> containingArchiveRootUrl(
    const QUrl &url, kiriview::RustArchiveRootPath (*rustFunction)(rust::Str, rust::Str))
{
    const QByteArray scheme = url.scheme().toUtf8();
    const QByteArray path = url.path().toUtf8();
    const kiriview::RustArchiveRootPath rootPath
        = rustFunction(kiriview::Bridge::rustStr(scheme), kiriview::Bridge::rustStr(path));
    if (!rootPath.found) {
        return std::nullopt;
    }

    QUrl archiveRootUrl = url;
    archiveRootUrl.setPath(kiriview::Bridge::qtString(rootPath.path));
    archiveRootUrl.setQuery(QString());
    archiveRootUrl.setFragment(QString());
    if (!archiveRootUrl.isValid() || archiveRootUrl.path().isEmpty()) {
        return std::nullopt;
    }

    return archiveRootUrl;
}

QUrl openedCollectionScopeSourceNavigationUrl(const kiriview::DisplayedImageLocation &location)
{
    return kiriview::normalizedFileContainerUrl(
        kiriview::navigationSourceUrl(location.openedCollectionScopeSourceUrl()));
}

bool openedCollectionScopeContainsUrlInRust(
    const kiriview::OpenedCollectionScopeLocation &openedCollectionScope, const QUrl &url)
{
    const UrlParts root = urlParts(openedCollectionScope.rootUrl());
    const UrlParts candidate = urlParts(url);
    return kiriview::rustOpenedCollectionScopeContainsUrl(openedCollectionScope.isEmpty(),
        root.empty, kiriview::Bridge::rustStr(root.scheme), kiriview::Bridge::rustStr(root.path),
        candidate.empty, kiriview::Bridge::rustStr(candidate.scheme),
        kiriview::Bridge::rustStr(candidate.path));
}

}

namespace kiriview {
std::optional<QUrl> comicBookArchiveRootUrl(const QUrl &url)
{
    const std::optional<ArchiveCollectionRoot> root
        = archiveCollectionRootForLocalArchive(url, kiriview::comicBookArchiveMatchForUrl(url));
    if (!root.has_value()) {
        return std::nullopt;
    }

    return root->rootUrl;
}

std::optional<QUrl> directArchiveOpenRootUrl(const QUrl &url)
{
    const std::optional<ArchiveCollectionRoot> root
        = directArchiveCollectionRootForLocalArchive(url);
    if (!root.has_value()) {
        return std::nullopt;
    }

    return root->rootUrl;
}

std::optional<OpenedCollectionScopeLocation> openedCollectionScopeLocationForLocalArchiveUrl(
    const QUrl &url)
{
    const std::optional<ArchiveCollectionRoot> root
        = directArchiveCollectionRootForLocalArchive(url);
    if (!root.has_value()) {
        return std::nullopt;
    }

    return OpenedCollectionScopeLocation::fromUrls(
        normalizedFileContainerUrl(url), root->rootUrl, root->kind);
}

std::optional<OpenedCollectionScopeLocation> openedCollectionScopeLocationForDirectlyOpenedLocalUrl(
    const QUrl &url)
{
    const std::optional<OpenedCollectionScopeLocation> directoryCollection
        = directoryOpenedCollectionScopeLocationForLocalUrl(url);
    if (directoryCollection.has_value()) {
        return directoryCollection;
    }

    const std::optional<OpenedCollectionScopeLocation> archiveCollection
        = openedCollectionScopeLocationForLocalArchiveUrl(url);
    if (archiveCollection.has_value()) {
        return archiveCollection;
    }

    return std::nullopt;
}

bool isUrlInsideArchiveRoot(const QUrl &url, const QUrl &archiveRootUrl)
{
    const UrlParts root = urlParts(archiveRootUrl);
    const UrlParts candidate = urlParts(url);
    return rustOpenedCollectionScopeContainsUrl(false, root.empty, Bridge::rustStr(root.scheme),
        Bridge::rustStr(root.path), candidate.empty, Bridge::rustStr(candidate.scheme),
        Bridge::rustStr(candidate.path));
}

bool openedCollectionScopeContainsUrl(
    const OpenedCollectionScopeLocation &openedCollectionScope, const QUrl &url)
{
    return openedCollectionScopeContainsUrlInRust(openedCollectionScope, url);
}

bool displayedLocationIsInsideOpenedCollectionScope(const DisplayedImageLocation &location)
{
    return openedCollectionScopeContainsUrl(location.openedCollectionScope(), location.imageUrl());
}

std::optional<QUrl> containingComicBookArchiveRootUrl(const QUrl &url)
{
    return containingArchiveRootUrl(url, rustContainingComicBookArchiveRootPath);
}

std::optional<QUrl> containingDirectArchiveOpenRootUrl(const QUrl &url)
{
    return containingArchiveRootUrl(url, rustContainingDirectArchiveOpenRootPath);
}

QString windowTitleFileNameForDisplayedLocation(const DisplayedImageLocation &location)
{
    if (location.imageUrl().isEmpty()) {
        return QString();
    }

    if (displayedLocationIsInsideOpenedCollectionScope(location)
        && !location.openedCollectionScopeSourceUrl().fileName().isEmpty()) {
        return location.openedCollectionScopeSourceUrl().fileName();
    }

    return location.imageUrl().fileName();
}

QUrl zoomScopeUrlForLocation(const DisplayedImageLocation &location)
{
    if (displayedLocationIsInsideOpenedCollectionScope(location)) {
        return openedCollectionScopeSourceNavigationUrl(location);
    }

    return {};
}

QUrl containerNavigationUrlForLocation(const DisplayedImageLocation &location)
{
    if (!location.openedCollectionScope().isComicBook()
        || !displayedLocationIsInsideOpenedCollectionScope(location)) {
        return {};
    }

    return openedCollectionScopeSourceNavigationUrl(location);
}

}
