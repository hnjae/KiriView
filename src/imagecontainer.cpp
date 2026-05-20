// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainer.h"

#include "archiveformat.h"
#include "imageurl.h"
#include "kiriview/src/archivepath.cxx.h"
#include "rustqtconversion.h"

#include <QByteArray>
#include <QFileInfo>
#include <optional>

namespace {
struct ArchiveDocumentRoot {
    QUrl rootUrl;
    KiriView::ArchiveDocumentKind kind = KiriView::ArchiveDocumentKind::General;
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

KiriView::ArchiveDocumentKind archiveDocumentKindForMatch(const KiriView::ArchiveOpenMatch &match)
{
    switch (match.kind) {
    case KiriView::ArchiveOpenMatchKind::ComicBook:
        return KiriView::ArchiveDocumentKind::ComicBook;
    case KiriView::ArchiveOpenMatchKind::GeneralArchive:
        return KiriView::ArchiveDocumentKind::General;
    }

    return KiriView::ArchiveDocumentKind::General;
}

std::optional<ArchiveDocumentRoot> archiveDocumentRootForLocalArchive(
    const QUrl &url, std::optional<KiriView::ArchiveOpenMatch> match)
{
    if (!match.has_value()) {
        return std::nullopt;
    }

    const std::optional<QUrl> rootUrl = archiveRootUrlForLocalArchive(url, match->scheme);
    if (!rootUrl.has_value()) {
        return std::nullopt;
    }

    return ArchiveDocumentRoot { *rootUrl, archiveDocumentKindForMatch(*match) };
}

std::optional<ArchiveDocumentRoot> directArchiveDocumentRootForLocalArchive(const QUrl &url)
{
    return archiveDocumentRootForLocalArchive(url, KiriView::directArchiveOpenMatchForUrl(url));
}

std::optional<KiriView::ArchiveDocumentLocation> directoryDocumentLocationForLocalUrl(
    const QUrl &url)
{
    if (!url.isLocalFile()) {
        return std::nullopt;
    }

    const QUrl fileUrl = KiriView::normalizedFileContainerUrl(url);
    const QString localPath = fileUrl.toLocalFile();
    if (localPath.isEmpty() || !QFileInfo(localPath).isDir()) {
        return std::nullopt;
    }

    return KiriView::ArchiveDocumentLocation::fromUrls(fileUrl,
        KiriView::normalizedDirectoryContainerUrl(fileUrl),
        KiriView::ArchiveDocumentKind::Directory);
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

QUrl archiveDocumentFileNavigationUrl(const KiriView::DisplayedImageLocation &location)
{
    return KiriView::normalizedFileContainerUrl(
        KiriView::navigationSourceUrl(location.archiveDocumentFileUrl()));
}

bool archiveDocumentContainsUrlInRust(
    const KiriView::ArchiveDocumentLocation &archiveDocument, const QUrl &url)
{
    const UrlParts root = urlParts(archiveDocument.rootUrl());
    const UrlParts candidate = urlParts(url);
    return KiriView::rustArchiveDocumentContainsUrl(archiveDocument.isEmpty(), root.empty,
        KiriView::Bridge::rustStr(root.scheme), KiriView::Bridge::rustStr(root.path),
        candidate.empty, KiriView::Bridge::rustStr(candidate.scheme),
        KiriView::Bridge::rustStr(candidate.path));
}

}

namespace KiriView {
std::optional<QUrl> comicBookArchiveRootUrl(const QUrl &url)
{
    const std::optional<ArchiveDocumentRoot> root
        = archiveDocumentRootForLocalArchive(url, KiriView::comicBookArchiveMatchForUrl(url));
    if (!root.has_value()) {
        return std::nullopt;
    }

    return root->rootUrl;
}

std::optional<QUrl> directArchiveOpenRootUrl(const QUrl &url)
{
    const std::optional<ArchiveDocumentRoot> root = directArchiveDocumentRootForLocalArchive(url);
    if (!root.has_value()) {
        return std::nullopt;
    }

    return root->rootUrl;
}

std::optional<ArchiveDocumentLocation> archiveDocumentLocationForLocalArchiveUrl(const QUrl &url)
{
    const std::optional<ArchiveDocumentRoot> root = directArchiveDocumentRootForLocalArchive(url);
    if (!root.has_value()) {
        return std::nullopt;
    }

    return ArchiveDocumentLocation::fromUrls(
        normalizedFileContainerUrl(url), root->rootUrl, root->kind);
}

std::optional<ArchiveDocumentLocation> directOpenDocumentLocationForLocalUrl(const QUrl &url)
{
    const std::optional<ArchiveDocumentLocation> directoryDocument
        = directoryDocumentLocationForLocalUrl(url);
    if (directoryDocument.has_value()) {
        return directoryDocument;
    }

    const std::optional<ArchiveDocumentLocation> archiveDocument
        = archiveDocumentLocationForLocalArchiveUrl(url);
    if (archiveDocument.has_value()) {
        return archiveDocument;
    }

    return std::nullopt;
}

bool isUrlInsideArchiveRoot(const QUrl &url, const QUrl &archiveRootUrl)
{
    const UrlParts root = urlParts(archiveRootUrl);
    const UrlParts candidate = urlParts(url);
    return rustArchiveDocumentContainsUrl(false, root.empty, Bridge::rustStr(root.scheme),
        Bridge::rustStr(root.path), candidate.empty, Bridge::rustStr(candidate.scheme),
        Bridge::rustStr(candidate.path));
}

bool archiveDocumentContainsUrl(const ArchiveDocumentLocation &archiveDocument, const QUrl &url)
{
    return archiveDocumentContainsUrlInRust(archiveDocument, url);
}

bool displayedLocationIsInsideArchiveDocument(const DisplayedImageLocation &location)
{
    return archiveDocumentContainsUrl(location.archiveDocument(), location.imageUrl());
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

    if (displayedLocationIsInsideArchiveDocument(location)
        && !location.archiveDocumentFileUrl().fileName().isEmpty()) {
        return location.archiveDocumentFileUrl().fileName();
    }

    return location.imageUrl().fileName();
}

QUrl zoomScopeUrlForLocation(const DisplayedImageLocation &location)
{
    if (displayedLocationIsInsideArchiveDocument(location)) {
        return archiveDocumentFileNavigationUrl(location);
    }

    return {};
}

QUrl containerNavigationUrlForLocation(const DisplayedImageLocation &location)
{
    if (!location.archiveDocument().isComicBook()
        || !displayedLocationIsInsideArchiveDocument(location)) {
        return {};
    }

    return archiveDocumentFileNavigationUrl(location);
}

}
