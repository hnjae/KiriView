// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainer.h"

#include "archiveformat.h"
#include "imageformatregistry.h"
#include "imagenavigationmodel.h"
#include "imageurl.h"

#include <QDir>
#include <QStringList>
#include <cstddef>

namespace {
QString normalizedArchiveRootPath(const QUrl &archiveRootUrl)
{
    QString path = QDir::cleanPath(archiveRootUrl.path());
    if (!path.endsWith(QLatin1Char('/'))) {
        path += QLatin1Char('/');
    }

    return path;
}

std::optional<QUrl> archiveFileUrl(const QUrl &archiveRootUrl)
{
    if (!KiriView::isSupportedArchiveRootScheme(archiveRootUrl.scheme())) {
        return std::nullopt;
    }

    QString archivePath = normalizedArchiveRootPath(archiveRootUrl);
    if (!archivePath.endsWith(QLatin1Char('/'))) {
        return std::nullopt;
    }

    archivePath.chop(1);
    if (archivePath.isEmpty()) {
        return std::nullopt;
    }

    return KiriView::normalizedFileContainerUrl(QUrl::fromLocalFile(archivePath));
}

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

QString archiveRelativeImageName(const QUrl &archiveRootUrl, const QUrl &imageUrl)
{
    const QString rootPath = normalizedArchiveRootPath(archiveRootUrl);
    const QString path = QDir::cleanPath(imageUrl.path());
    if (!path.startsWith(rootPath)) {
        return imageUrl.fileName();
    }

    return path.mid(rootPath.size());
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
    const QString archiveScheme = KiriView::directArchiveOpenKioSchemeForUrl(url);
    return archiveRootUrlForLocalArchive(url, archiveScheme);
}

std::optional<ArchiveDocumentLocation> archiveDocumentLocationForLocalArchiveUrl(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return std::nullopt;
    }

    const std::optional<QUrl> rootUrl = directArchiveOpenRootUrl(url);
    if (!rootUrl.has_value()) {
        return std::nullopt;
    }

    const ArchiveDocumentKind kind = isComicBookArchiveUrl(url) ? ArchiveDocumentKind::ComicBook
                                                                : ArchiveDocumentKind::General;
    return ArchiveDocumentLocation::fromUrls(normalizedFileContainerUrl(url), *rootUrl, kind);
}

bool isUrlInsideArchiveRoot(const QUrl &url, const QUrl &archiveRootUrl)
{
    if (url.isEmpty() || archiveRootUrl.isEmpty() || url.scheme() != archiveRootUrl.scheme()) {
        return false;
    }

    const QString rootPath = normalizedArchiveRootPath(archiveRootUrl);
    const QString path = QDir::cleanPath(url.path());
    return path.size() > rootPath.size() && path.startsWith(rootPath);
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

void appendArchiveImageNavigationCandidates(std::vector<ImageNavigationCandidate> *candidates,
    const KIO::UDSEntryList &entries, const QUrl &directoryUrl, const QUrl &archiveRootUrl)
{
    candidates->reserve(candidates->size() + static_cast<std::size_t>(entries.size()));

    for (const KIO::UDSEntry &entry : entries) {
        const KFileItem item(entry, directoryUrl, true, true);
        const QString name = item.name();
        if (!item.isFile() || !KiriView::isSupportedImageFileName(name)) {
            continue;
        }

        const QUrl imageUrl = KiriView::normalizedImageUrl(item.url());
        candidates->push_back(ImageNavigationCandidate {
            imageUrl, archiveRelativeImageName(archiveRootUrl, imageUrl) });
    }
}

QUrl imageContainerUrlForImage(const QUrl &imageUrl, const QUrl &archiveRootUrl)
{
    if (imageUrl.isEmpty()) {
        return {};
    }

    if (isUrlInsideArchiveRoot(imageUrl, archiveRootUrl)) {
        const std::optional<QUrl> archiveUrl = archiveFileUrl(archiveRootUrl);
        if (!archiveUrl.has_value()) {
            return {};
        }

        return normalizedFileContainerUrl(navigationSourceUrl(*archiveUrl));
    }

    const QUrl currentUrl = navigationSourceUrl(imageUrl);
    const QUrl parentUrl = currentUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        return {};
    }

    return normalizedDirectoryContainerUrl(parentUrl);
}

QUrl imageContainerUrlForLocation(const DisplayedImageLocation &location)
{
    if (displayedLocationIsInsideArchiveDocument(location)) {
        return normalizedFileContainerUrl(navigationSourceUrl(location.archiveDocumentFileUrl()));
    }

    return imageContainerUrlForImage(location.imageUrl(), QUrl());
}

QUrl containerNavigationUrlForLocation(const DisplayedImageLocation &location)
{
    if (!location.archiveDocument().isComicBook()
        || !displayedLocationIsInsideArchiveDocument(location)) {
        return {};
    }

    return normalizedFileContainerUrl(navigationSourceUrl(location.archiveDocumentFileUrl()));
}
}
