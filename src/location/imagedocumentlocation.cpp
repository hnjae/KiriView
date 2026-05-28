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
    KiriView::OpenedCollectionScopeKind kind = KiriView::OpenedCollectionScopeKind::GeneralArchive;
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
    const KiriView::RustArchiveRootPath rootPath = KiriView::rustArchiveRootPathForLocalArchive(
        url.isLocalFile(), KiriView::Bridge::rustStr(archiveSchemeBytes),
        KiriView::Bridge::rustStr(localPathBytes));
    if (!rootPath.found) {
        return std::nullopt;
    }

    QUrl archiveRootUrl;
    archiveRootUrl.setScheme(archiveScheme);
    archiveRootUrl.setPath(KiriView::Bridge::qtString(rootPath.path));
    if (!archiveRootUrl.isValid() || archiveRootUrl.path().isEmpty()) {
        return std::nullopt;
    }

    return archiveRootUrl;
}

KiriView::OpenedCollectionScopeKind archiveCollectionKindForMatch(
    const KiriView::ArchiveOpenMatch &match)
{
    switch (match.kind) {
    case KiriView::ArchiveOpenMatchKind::ComicBook:
        return KiriView::OpenedCollectionScopeKind::ComicBookArchive;
    case KiriView::ArchiveOpenMatchKind::GeneralArchive:
        return KiriView::OpenedCollectionScopeKind::GeneralArchive;
    }

    return KiriView::OpenedCollectionScopeKind::GeneralArchive;
}

std::optional<ArchiveCollectionRoot> archiveCollectionRootForLocalArchive(
    const QUrl &url, std::optional<KiriView::ArchiveOpenMatch> match)
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
    return archiveCollectionRootForLocalArchive(url, KiriView::directArchiveOpenMatchForUrl(url));
}

std::optional<KiriView::OpenedCollectionScopeLocation>
directoryOpenedCollectionScopeLocationForLocalUrl(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return std::nullopt;
    }

    const QUrl fileUrl = KiriView::normalizedFileContainerUrl(url);
    const QString localPath = fileUrl.toLocalFile();
    if (localPath.isEmpty() || !QFileInfo(localPath).isDir()) {
        return std::nullopt;
    }

    return KiriView::OpenedCollectionScopeLocation::fromUrls(fileUrl,
        KiriView::normalizedDirectoryContainerUrl(fileUrl),
        KiriView::OpenedCollectionScopeKind::Directory);
}

std::optional<QUrl> containingArchiveRootUrl(
    const QUrl &url, KiriView::RustArchiveRootPath (*rustFunction)(rust::Str, rust::Str))
{
    const QByteArray scheme = url.scheme().toUtf8();
    const QByteArray path = url.path().toUtf8();
    const KiriView::RustArchiveRootPath rootPath
        = rustFunction(KiriView::Bridge::rustStr(scheme), KiriView::Bridge::rustStr(path));
    if (!rootPath.found) {
        return std::nullopt;
    }

    QUrl archiveRootUrl = url;
    archiveRootUrl.setPath(KiriView::Bridge::qtString(rootPath.path));
    archiveRootUrl.setQuery(QString());
    archiveRootUrl.setFragment(QString());
    if (!archiveRootUrl.isValid() || archiveRootUrl.path().isEmpty()) {
        return std::nullopt;
    }

    return archiveRootUrl;
}

QUrl openedCollectionScopeSourceNavigationUrl(const KiriView::DisplayedImageLocation &location)
{
    return KiriView::normalizedFileContainerUrl(
        KiriView::navigationSourceUrl(location.openedCollectionScopeSourceUrl()));
}

bool openedCollectionScopeContainsUrlInRust(
    const KiriView::OpenedCollectionScopeLocation &openedCollectionScope, const QUrl &url)
{
    const UrlParts root = urlParts(openedCollectionScope.rootUrl());
    const UrlParts candidate = urlParts(url);
    return KiriView::rustOpenedCollectionScopeContainsUrl(openedCollectionScope.isEmpty(),
        root.empty, KiriView::Bridge::rustStr(root.scheme), KiriView::Bridge::rustStr(root.path),
        candidate.empty, KiriView::Bridge::rustStr(candidate.scheme),
        KiriView::Bridge::rustStr(candidate.path));
}

}

namespace KiriView {
std::optional<QUrl> comicBookArchiveRootUrl(const QUrl &url)
{
    const std::optional<ArchiveCollectionRoot> root
        = archiveCollectionRootForLocalArchive(url, KiriView::comicBookArchiveMatchForUrl(url));
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
