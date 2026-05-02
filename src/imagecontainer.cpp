// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainer.h"

#include "imageformatregistry.h"
#include "imageurl.h"

#include <QCollator>
#include <QDir>
#include <QLocale>
#include <Qt>
#include <algorithm>
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

std::optional<QUrl> comicBookArchiveFileUrl(const QUrl &archiveRootUrl)
{
    if (archiveRootUrl.scheme() != QStringLiteral("zip")) {
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

void sortContainerNavigationCandidates(
    std::vector<KiriView::ContainerNavigationCandidate> *candidates)
{
    QCollator collator(QLocale::system());
    collator.setCaseSensitivity(Qt::CaseSensitive);
    collator.setNumericMode(false);
    collator.setIgnorePunctuation(false);

    std::stable_sort(candidates->begin(), candidates->end(),
        [&collator](const KiriView::ContainerNavigationCandidate &left,
            const KiriView::ContainerNavigationCandidate &right) {
            const int nameComparison = collator.compare(left.name, right.name);
            if (nameComparison != 0) {
                return nameComparison < 0;
            }

            return left.url.toEncoded() < right.url.toEncoded();
        });

    const auto duplicateStart = std::unique(candidates->begin(), candidates->end(),
        [](const KiriView::ContainerNavigationCandidate &left,
            const KiriView::ContainerNavigationCandidate &right) {
            return left.url.matches(right.url, QUrl::NormalizePathSegments);
        });
    candidates->erase(duplicateStart, candidates->end());
}
}

namespace KiriView {
std::optional<QUrl> comicBookArchiveRootUrl(const QUrl &url)
{
    if (!KiriView::isComicBookArchiveUrl(url)) {
        return std::nullopt;
    }

    const QString localPath = QDir::cleanPath(url.toLocalFile());
    if (localPath.isEmpty()) {
        return std::nullopt;
    }

    QUrl archiveRootUrl;
    archiveRootUrl.setScheme(QStringLiteral("zip"));
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
    if (url.scheme() != QStringLiteral("zip")) {
        return std::nullopt;
    }

    const QString path = QDir::cleanPath(url.path());
    const QString foldedPath = path.toCaseFolded();
    const QString marker = QStringLiteral(".cbz/");
    const qsizetype markerIndex = foldedPath.indexOf(marker);
    if (markerIndex < 0) {
        return std::nullopt;
    }

    QUrl archiveRootUrl = url;
    archiveRootUrl.setPath(path.left(markerIndex + 4) + QLatin1Char('/'));
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

        const QUrl imageUrl = item.url().adjusted(QUrl::NormalizePathSegments);
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
