// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainer.h"

#include "archiveformat.h"
#include "archivepath.h"
#include "imageformatregistry.h"
#include "imagenavigationmodel.h"
#include "imageurl.h"

#include <QDir>
#include <QStringList>
#include <cstddef>
#include <optional>

namespace {
struct ArchiveDocumentRoot {
    QUrl rootUrl;
    KiriView::ArchiveDocumentKind kind = KiriView::ArchiveDocumentKind::General;
};

std::optional<QUrl> archiveRootUrlForLocalArchive(const QUrl &url, const QString &archiveScheme)
{
    if (!url.isLocalFile()) {
        return std::nullopt;
    }

    if (archiveScheme.isEmpty()) {
        return std::nullopt;
    }

    const QString localPath = QDir::cleanPath(url.toLocalFile());
    if (localPath.isEmpty()) {
        return std::nullopt;
    }

    QUrl archiveRootUrl;
    archiveRootUrl.setScheme(archiveScheme);
    archiveRootUrl.setPath(localPath + QLatin1Char('/'));
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

std::optional<ArchiveDocumentRoot> archiveDocumentRootForLocalArchive(const QUrl &url)
{
    const std::optional<KiriView::ArchiveOpenMatch> match
        = KiriView::directArchiveOpenMatchForUrl(url);
    if (!match.has_value()) {
        return std::nullopt;
    }

    const std::optional<QUrl> rootUrl = archiveRootUrlForLocalArchive(url, match->scheme);
    if (!rootUrl.has_value()) {
        return std::nullopt;
    }

    return ArchiveDocumentRoot { *rootUrl, archiveDocumentKindForMatch(*match) };
}

std::optional<QUrl> containingArchiveRootUrl(const QUrl &url, const QStringList &archiveMarkers)
{
    if (archiveMarkers.isEmpty()) {
        return std::nullopt;
    }

    const QString path = QDir::cleanPath(url.path());
    const QString foldedPath = path.toCaseFolded();
    qsizetype markerIndex = -1;
    qsizetype markerSize = 0;
    for (const QString &marker : archiveMarkers) {
        const qsizetype candidateIndex = foldedPath.indexOf(marker);
        if (candidateIndex < 0 || (markerIndex >= 0 && candidateIndex >= markerIndex)) {
            continue;
        }

        markerIndex = candidateIndex;
        markerSize = marker.size();
    }

    if (markerIndex < 0) {
        return std::nullopt;
    }

    QUrl archiveRootUrl = url;
    archiveRootUrl.setPath(path.left(markerIndex + markerSize - 1) + QLatin1Char('/'));
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

}

namespace KiriView {
std::optional<QUrl> comicBookArchiveRootUrl(const QUrl &url)
{
    const QString archiveScheme = KiriView::comicBookArchiveKioSchemeForUrl(url);
    return archiveRootUrlForLocalArchive(url, archiveScheme);
}

std::optional<QUrl> directArchiveOpenRootUrl(const QUrl &url)
{
    const std::optional<ArchiveDocumentRoot> root = archiveDocumentRootForLocalArchive(url);
    if (!root.has_value()) {
        return std::nullopt;
    }

    return root->rootUrl;
}

std::optional<ArchiveDocumentLocation> archiveDocumentLocationForLocalArchiveUrl(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return std::nullopt;
    }

    const std::optional<ArchiveDocumentRoot> root = archiveDocumentRootForLocalArchive(url);
    if (!root.has_value()) {
        return std::nullopt;
    }

    return ArchiveDocumentLocation::fromUrls(
        normalizedFileContainerUrl(url), root->rootUrl, root->kind);
}

bool isUrlInsideArchiveRoot(const QUrl &url, const QUrl &archiveRootUrl)
{
    return !archiveRelativePathForUrl(archiveRootUrl, url).isEmpty();
}

bool archiveDocumentContainsUrl(const ArchiveDocumentLocation &archiveDocument, const QUrl &url)
{
    return !archiveDocument.isEmpty() && isUrlInsideArchiveRoot(url, archiveDocument.rootUrl());
}

bool displayedLocationIsInsideArchiveDocument(const DisplayedImageLocation &location)
{
    return archiveDocumentContainsUrl(location.archiveDocument(), location.imageUrl());
}

std::optional<QUrl> containingComicBookArchiveRootUrl(const QUrl &url)
{
    const QString marker = KiriView::comicBookArchiveMarkerForRootScheme(url.scheme());
    return containingArchiveRootUrl(url, marker.isEmpty() ? QStringList() : QStringList { marker });
}

std::optional<QUrl> containingDirectArchiveOpenRootUrl(const QUrl &url)
{
    return containingArchiveRootUrl(
        url, KiriView::directArchiveOpenMarkersForRootScheme(url.scheme()));
}

QString windowTitleFileNameForDisplayedLocation(const DisplayedImageLocation &location)
{
    if (location.imageUrl().isEmpty()) {
        return {};
    }

    if (displayedLocationIsInsideArchiveDocument(location)) {
        const QString archiveName = location.archiveDocumentFileUrl().fileName();
        if (!archiveName.isEmpty()) {
            return archiveName;
        }
    }

    return location.imageUrl().fileName();
}

std::vector<ContainerNavigationCandidate> containerNavigationCandidates(const KFileItemList &items)
{
    std::vector<ContainerNavigationCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(items.size()));

    for (const KFileItem &item : items) {
        const QString name = item.name();
        if (item.isFile() && item.url().isLocalFile()
            && KiriView::isComicBookArchiveFileName(name)) {
            candidates.push_back(
                ContainerNavigationCandidate { normalizedFileContainerUrl(item.url()), name,
                    ContainerNavigationCandidateType::ComicBookArchive });
        }
    }

    sortContainerNavigationCandidates(&candidates);
    return candidates;
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
