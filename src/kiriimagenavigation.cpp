// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagenavigation.h"

#include <QByteArray>
#include <QCollator>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStringList>
#include <Qt>
#include <QtGlobal>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <sys/types.h>
#include <sys/xattr.h>
#include <utility>

namespace {
constexpr const char *documentPortalHostPathAttribute = "user.document-portal.host-path";
constexpr std::ptrdiff_t predecodePreviousImageCount = 2;
constexpr std::ptrdiff_t predecodeNextImageCount = 10;

QUrl normalizedFileContainerUrl(const QUrl &url)
{
    QUrl normalizedUrl = url.adjusted(QUrl::NormalizePathSegments);
    normalizedUrl.setQuery(QString());
    normalizedUrl.setFragment(QString());

    if (normalizedUrl.isLocalFile()) {
        normalizedUrl = QUrl::fromLocalFile(QDir::cleanPath(normalizedUrl.toLocalFile()));
    }
    return normalizedUrl;
}

QUrl normalizedDirectoryContainerUrl(const QUrl &url)
{
    QUrl normalizedUrl = url.adjusted(QUrl::NormalizePathSegments);
    normalizedUrl.setQuery(QString());
    normalizedUrl.setFragment(QString());

    QString path = normalizedUrl.path();
    if (!path.endsWith(QLatin1Char('/'))) {
        path += QLatin1Char('/');
        normalizedUrl.setPath(path);
    }
    return normalizedUrl;
}

bool isSupportedImageFileName(const QString &name)
{
    static const QStringList supportedExtensions = {
        QStringLiteral("avif"),
        QStringLiteral("avifs"),
        QStringLiteral("bmp"),
        QStringLiteral("gif"),
        QStringLiteral("heic"),
        QStringLiteral("heif"),
        QStringLiteral("hif"),
        QStringLiteral("jpeg"),
        QStringLiteral("jpg"),
        QStringLiteral("jp2"),
        QStringLiteral("jxl"),
        QStringLiteral("png"),
        QStringLiteral("svg"),
        QStringLiteral("webp"),
    };

    const qsizetype dotIndex = name.lastIndexOf(QLatin1Char('.'));
    if (dotIndex <= 0 || dotIndex == name.size() - 1) {
        return false;
    }

    return supportedExtensions.contains(name.mid(dotIndex + 1).toCaseFolded());
}

bool isComicBookArchiveFileName(const QString &name)
{
    const qsizetype dotIndex = name.lastIndexOf(QLatin1Char('.'));
    if (dotIndex <= 0 || dotIndex == name.size() - 1) {
        return false;
    }

    return name.mid(dotIndex + 1).toCaseFolded() == QStringLiteral("cbz");
}

bool isComicBookArchiveUrl(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return false;
    }

    if (isComicBookArchiveFileName(url.fileName())) {
        return true;
    }

    const QMimeType mimeType
        = QMimeDatabase().mimeTypeForFile(url.toLocalFile(), QMimeDatabase::MatchExtension);
    return mimeType.name() == QStringLiteral("application/vnd.comicbook+zip");
}

bool isKioFuseArchiveScheme(const QString &scheme)
{
    static const QStringList archiveSchemes = {
        QStringLiteral("zip"),
    };

    return archiveSchemes.contains(scheme);
}

std::optional<QUrl> kioFuseArchiveUrl(const QString &localPath)
{
    const QString cleanPath = QDir::cleanPath(localPath);
    const QString runtimeDir = QFile::decodeName(qgetenv("XDG_RUNTIME_DIR"));
    const QString marker = QStringLiteral("/kio-fuse-");
    qsizetype markerIndex = -1;

    if (!runtimeDir.isEmpty()) {
        const QString prefix = QDir::cleanPath(runtimeDir) + marker;
        if (!cleanPath.startsWith(prefix)) {
            return std::nullopt;
        }
        markerIndex = prefix.size() - marker.size();
    } else {
        markerIndex = cleanPath.indexOf(marker);
        if (markerIndex < 0) {
            return std::nullopt;
        }
    }

    const qsizetype mountEnd = cleanPath.indexOf(QLatin1Char('/'), markerIndex + marker.size());
    if (mountEnd < 0 || mountEnd == cleanPath.size() - 1) {
        return std::nullopt;
    }

    const QString relativePath = cleanPath.mid(mountEnd + 1);
    const qsizetype schemeEnd = relativePath.indexOf(QLatin1Char('/'));
    if (schemeEnd <= 0 || schemeEnd == relativePath.size() - 1) {
        return std::nullopt;
    }

    const QString scheme = relativePath.left(schemeEnd);
    if (!isKioFuseArchiveScheme(scheme)) {
        return std::nullopt;
    }

    QUrl url;
    url.setScheme(scheme);
    url.setPath(relativePath.mid(schemeEnd));
    if (!url.isValid() || url.path().isEmpty()) {
        return std::nullopt;
    }

    return url;
}

QUrl navigationUrlForLocalPath(const QString &localPath)
{
    const std::optional<QUrl> kioUrl = kioFuseArchiveUrl(localPath);
    if (kioUrl.has_value()) {
        return kioUrl.value();
    }

    return QUrl::fromLocalFile(localPath);
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

    return normalizedFileContainerUrl(QUrl::fromLocalFile(archivePath));
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

template <typename Candidate>
std::optional<std::size_t> adjacentCandidateIndex(const std::vector<Candidate> &candidates,
    const QUrl &currentUrl, KiriView::NavigationDirection direction)
{
    const auto current = std::find_if(
        candidates.cbegin(), candidates.cend(), [&currentUrl](const Candidate &candidate) {
            return candidate.url.matches(currentUrl, QUrl::NormalizePathSegments);
        });
    if (current == candidates.cend()) {
        return std::nullopt;
    }

    const auto currentIndex = std::distance(candidates.cbegin(), current);
    if (direction == KiriView::NavigationDirection::Previous) {
        if (currentIndex == 0) {
            return std::nullopt;
        }
        return static_cast<std::size_t>(currentIndex - 1);
    }

    const auto nextIndex = currentIndex + 1;
    if (nextIndex == static_cast<std::ptrdiff_t>(candidates.size())) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(nextIndex);
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

std::optional<QUrl> documentPortalHostUrl(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return std::nullopt;
    }

    const QString localPath = url.toLocalFile();
    const QByteArray encodedLocalPath = QFile::encodeName(localPath);
    if (encodedLocalPath.isEmpty()) {
        return std::nullopt;
    }

    // File dialogs can return document-portal URLs; navigation needs the real directory.
    const ssize_t valueSize
        = getxattr(encodedLocalPath.constData(), documentPortalHostPathAttribute, nullptr, 0);
    if (valueSize <= 0) {
        return std::nullopt;
    }

    QByteArray value;
    value.resize(valueSize);

    const ssize_t bytesRead = getxattr(encodedLocalPath.constData(),
        documentPortalHostPathAttribute, value.data(), static_cast<std::size_t>(value.size()));
    if (bytesRead <= 0) {
        return std::nullopt;
    }

    value.resize(bytesRead);
    if (value.endsWith('\0')) {
        value.chop(1);
    }

    const QString hostPath = QFile::decodeName(value);
    if (hostPath.isEmpty() || hostPath == localPath) {
        return std::nullopt;
    }

    return navigationUrlForLocalPath(hostPath);
}
}

namespace KiriView {
QUrl normalizedImageUrl(const QUrl &url) { return url.adjusted(QUrl::NormalizePathSegments); }

QUrl parentUrlForContainerNavigation(const QUrl &containerUrl)
{
    QUrl parentSourceUrl = containerUrl.adjusted(QUrl::NormalizePathSegments);
    QString path = parentSourceUrl.path();
    if (path.size() > 1 && path.endsWith(QLatin1Char('/'))) {
        path.chop(1);
        parentSourceUrl.setPath(path);
    }

    return parentSourceUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
}

std::vector<QUrl> predecodeWindowImageUrls(
    const std::vector<ImageNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    std::vector<QUrl> urls;
    const auto current = std::find_if(candidates.cbegin(), candidates.cend(),
        [&currentUrl](const ImageNavigationCandidate &candidate) {
            return candidate.url.matches(currentUrl, QUrl::NormalizePathSegments);
        });
    if (current == candidates.cend()) {
        return urls;
    }

    const auto currentIndex = std::distance(candidates.cbegin(), current);
    auto appendOffset = [&urls, &candidates, currentIndex](std::ptrdiff_t offset) {
        const std::ptrdiff_t targetIndex = currentIndex + offset;
        if (targetIndex < 0 || targetIndex >= static_cast<std::ptrdiff_t>(candidates.size())) {
            return;
        }

        urls.push_back(candidates.at(static_cast<std::size_t>(targetIndex)).url);
    };

    appendOffset(0);
    for (std::ptrdiff_t offset = 1; offset <= predecodeNextImageCount; ++offset) {
        appendOffset(offset);
        if (offset <= predecodePreviousImageCount) {
            appendOffset(-offset);
        }
    }
    return urls;
}

std::vector<QUrl> imageNavigationCandidateUrls(
    const std::vector<ImageNavigationCandidate> &candidates)
{
    std::vector<QUrl> urls;
    urls.reserve(candidates.size());

    for (const ImageNavigationCandidate &candidate : candidates) {
        urls.push_back(candidate.url);
    }
    return urls;
}

std::optional<QUrl> adjacentImageNavigationUrl(
    const std::vector<ImageNavigationCandidate> &candidates, const QUrl &currentUrl,
    NavigationDirection direction)
{
    const std::optional<std::size_t> targetIndex
        = adjacentCandidateIndex(candidates, currentUrl, direction);
    if (!targetIndex.has_value()) {
        return std::nullopt;
    }

    return candidates.at(*targetIndex).url;
}

std::optional<ContainerNavigationCandidate> adjacentContainerNavigationCandidate(
    const std::vector<ContainerNavigationCandidate> &candidates, const QUrl &currentContainerUrl,
    NavigationDirection direction)
{
    const std::optional<std::size_t> targetIndex
        = adjacentCandidateIndex(candidates, currentContainerUrl, direction);
    if (!targetIndex.has_value()) {
        return std::nullopt;
    }

    return candidates.at(*targetIndex);
}

PageNavigationState pageNavigationStateForUrls(std::vector<QUrl> urls, const QUrl &currentUrl)
{
    PageNavigationState state { std::move(urls), -1 };
    const auto matchesCurrentUrl = [&currentUrl](const QUrl &url) {
        return url.matches(currentUrl, QUrl::NormalizePathSegments);
    };
    const auto current = std::find_if(state.urls.cbegin(), state.urls.cend(), matchesCurrentUrl);

    if (current == state.urls.cend()) {
        if (currentUrl.isValid() && !currentUrl.isEmpty()) {
            state.urls.insert(state.urls.begin(), currentUrl.adjusted(QUrl::NormalizePathSegments));
            state.currentIndex = 0;
        }
    } else {
        state.currentIndex = static_cast<int>(std::distance(state.urls.cbegin(), current));
    }

    return state;
}

std::optional<QUrl> comicBookArchiveRootUrl(const QUrl &url)
{
    if (!isComicBookArchiveUrl(url)) {
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

void sortImageNavigationCandidates(std::vector<ImageNavigationCandidate> *candidates)
{
    QCollator collator(QLocale::system());
    collator.setCaseSensitivity(Qt::CaseSensitive);
    collator.setNumericMode(false);
    collator.setIgnorePunctuation(false);

    std::stable_sort(candidates->begin(), candidates->end(),
        [&collator](const ImageNavigationCandidate &left, const ImageNavigationCandidate &right) {
            const int nameComparison = collator.compare(left.name, right.name);
            if (nameComparison != 0) {
                return nameComparison < 0;
            }

            return left.url.toEncoded() < right.url.toEncoded();
        });

    const auto duplicateStart = std::unique(candidates->begin(), candidates->end(),
        [](const ImageNavigationCandidate &left, const ImageNavigationCandidate &right) {
            return left.url.matches(right.url, QUrl::NormalizePathSegments);
        });
    candidates->erase(duplicateStart, candidates->end());
}

std::vector<ImageNavigationCandidate> imageNavigationCandidates(const KFileItemList &items)
{
    std::vector<ImageNavigationCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(items.size()));

    for (const KFileItem &item : items) {
        const QString name = item.name();
        if (!item.isFile() || !isSupportedImageFileName(name)) {
            continue;
        }

        candidates.push_back(ImageNavigationCandidate { item.url(), name });
    }

    sortImageNavigationCandidates(&candidates);
    return candidates;
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

        if (item.isFile() && item.url().isLocalFile() && isComicBookArchiveFileName(name)) {
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
        if (!item.isFile() || !isSupportedImageFileName(name)) {
            continue;
        }

        const QUrl imageUrl = item.url().adjusted(QUrl::NormalizePathSegments);
        candidates->push_back(ImageNavigationCandidate {
            imageUrl, archiveRelativeImageName(archiveRootUrl, imageUrl) });
    }
}

QUrl navigationSourceUrl(const QUrl &url)
{
    const std::optional<QUrl> hostUrl = documentPortalHostUrl(url);
    if (hostUrl.has_value()) {
        return hostUrl.value();
    }

    if (url.isLocalFile()) {
        const std::optional<QUrl> kioUrl = kioFuseArchiveUrl(url.toLocalFile());
        if (kioUrl.has_value()) {
            return kioUrl.value();
        }
    }

    return url;
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

bool sameContainerNavigationUrl(const QUrl &left, const QUrl &right)
{
    return !left.isEmpty() && !right.isEmpty() && left.matches(right, QUrl::NormalizePathSegments);
}
}
