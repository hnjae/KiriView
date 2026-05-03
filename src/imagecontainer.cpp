// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainer.h"

#include "imageformatregistry.h"
#include "imagenavigationmodel.h"
#include "imageurl.h"

#include <QDir>
#include <QStringList>
#include <cstddef>

namespace {
bool isComicBookArchiveRootScheme(const QString &scheme)
{
    static const QStringList archiveSchemes = {
        QStringLiteral("zip"),
        QStringLiteral("tar"),
        QStringLiteral("sevenz"),
    };

    return archiveSchemes.contains(scheme);
}

QString comicBookArchiveMarkerForRootScheme(const QString &scheme)
{
    if (scheme == QStringLiteral("zip")) {
        return QStringLiteral(".cbz/");
    }
    if (scheme == QStringLiteral("tar")) {
        return QStringLiteral(".cbt/");
    }
    if (scheme == QStringLiteral("sevenz")) {
        return QStringLiteral(".cb7/");
    }

    return {};
}

QString normalizedArchiveRootPath(const QUrl &archiveRootUrl)
{
    QString path = QDir::cleanPath(archiveRootUrl.path());
    if (!path.endsWith(QLatin1Char('/'))) {
        path += QLatin1Char('/');
    }

    return path;
}

std::optional<QUrl> comicBookArchiveFileUrl(const QUrl &archiveRootUrl)
{
    if (!isComicBookArchiveRootScheme(archiveRootUrl.scheme())) {
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

bool isUrlInsideArchiveRoot(const QUrl &url, const QUrl &archiveRootUrl)
{
    if (url.isEmpty() || archiveRootUrl.isEmpty() || url.scheme() != archiveRootUrl.scheme()) {
        return false;
    }

    const QString rootPath = normalizedArchiveRootPath(archiveRootUrl);
    const QString path = QDir::cleanPath(url.path());
    return path.size() > rootPath.size() && path.startsWith(rootPath);
}

std::optional<QUrl> containingComicBookArchiveRootUrl(const QUrl &url)
{
    const QString marker = comicBookArchiveMarkerForRootScheme(url.scheme());
    if (marker.isEmpty()) {
        return std::nullopt;
    }

    const QString path = QDir::cleanPath(url.path());
    const QString foldedPath = path.toCaseFolded();
    const qsizetype markerIndex = foldedPath.indexOf(marker);
    if (markerIndex < 0) {
        return std::nullopt;
    }

    QUrl archiveRootUrl = url;
    archiveRootUrl.setPath(path.left(markerIndex + marker.size() - 1) + QLatin1Char('/'));
    archiveRootUrl.setQuery(QString());
    archiveRootUrl.setFragment(QString());
    if (!archiveRootUrl.isValid() || archiveRootUrl.path().isEmpty()) {
        return std::nullopt;
    }

    return archiveRootUrl;
}

QString windowTitleFileNameForDisplayedUrl(
    const QUrl &displayedUrl, const QUrl &displayedComicBookRootUrl)
{
    if (displayedUrl.isEmpty()) {
        return {};
    }

    if (isUrlInsideArchiveRoot(displayedUrl, displayedComicBookRootUrl)) {
        const std::optional<QUrl> archiveUrl = comicBookArchiveFileUrl(displayedComicBookRootUrl);
        if (archiveUrl.has_value()) {
            const QString archiveName = archiveUrl->fileName();
            if (!archiveName.isEmpty()) {
                return archiveName;
            }
        }
    }

    return displayedUrl.fileName();
}

std::vector<ContainerNavigationCandidate> containerNavigationCandidates(const KFileItemList &items)
{
    std::vector<ContainerNavigationCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(items.size()));

    for (const KFileItem &item : items) {
        const QString name = item.name();
        if (item.isDir()) {
            candidates.push_back(
                ContainerNavigationCandidate { normalizedDirectoryContainerUrl(item.url()), name,
                    ContainerNavigationCandidateType::Directory });
            continue;
        }

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

QUrl containerNavigationUrlForImage(const QUrl &imageUrl, const QUrl &comicBookRootUrl)
{
    if (imageUrl.isEmpty()) {
        return {};
    }

    if (isUrlInsideArchiveRoot(imageUrl, comicBookRootUrl)) {
        const std::optional<QUrl> archiveUrl = comicBookArchiveFileUrl(comicBookRootUrl);
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
}
