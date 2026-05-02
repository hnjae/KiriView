// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidaterepository.h"

#include "imagecontainer.h"
#include "imageiojobs.h"
#include "imageurl.h"

#include <utility>
#include <vector>

namespace KiriView {
ImageNavigationCandidateProvider defaultImageNavigationCandidateProvider()
{
    return ImageNavigationCandidateProvider {
        startDirectoryImageCandidateList,
        startDirectoryContainerCandidateList,
        startArchiveImageCandidateList,
    };
}

std::optional<ImageCandidateListContext> imageCandidateListContextForDisplayedImage(
    const QUrl &displayedUrl, const QUrl &comicBookRootUrl)
{
    if (displayedUrl.isEmpty()) {
        return std::nullopt;
    }

    if (isUrlInsideArchiveRoot(displayedUrl, comicBookRootUrl)) {
        const QUrl currentUrl = displayedUrl.adjusted(QUrl::NormalizePathSegments);
        if (!currentUrl.isValid() || comicBookRootUrl.isEmpty()) {
            return std::nullopt;
        }

        return ImageCandidateListContext {
            currentUrl,
            comicBookRootUrl,
            comicBookRootUrl,
            ImageCandidateContainerType::ComicBookArchive,
        };
    }

    const QUrl currentUrl = navigationSourceUrl(displayedUrl);
    const QUrl parentUrl = currentUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
    if (!currentUrl.isValid() || currentUrl.isEmpty() || !parentUrl.isValid()
        || parentUrl.isEmpty()) {
        return std::nullopt;
    }

    return ImageCandidateListContext {
        currentUrl,
        parentUrl,
        QUrl(),
        ImageCandidateContainerType::Directory,
    };
}

ImageCandidateRepository::ImageCandidateRepository()
    : m_provider(defaultImageNavigationCandidateProvider())
{
}

ImageCandidateRepository::ImageCandidateRepository(ImageNavigationCandidateProvider provider)
    : m_provider(std::move(provider))
{
}

ImageIoJob ImageCandidateRepository::loadImages(QObject *receiver,
    const ImageCandidateListContext &context, ImageCandidatesCallback callback,
    ErrorCallback errorCallback) const
{
    if (context.containerType == ImageCandidateContainerType::ComicBookArchive) {
        return loadArchiveImages(
            receiver, context.listUrl, std::move(callback), std::move(errorCallback));
    }

    return loadDirectoryImages(
        receiver, context.listUrl, std::move(callback), std::move(errorCallback));
}

ImageIoJob ImageCandidateRepository::loadDirectoryImages(QObject *receiver,
    const QUrl &directoryUrl, ImageCandidatesCallback callback, ErrorCallback errorCallback) const
{
    if (!m_provider.directoryImages) {
        if (errorCallback) {
            errorCallback(QString());
        }
        return ImageIoJob();
    }

    return m_provider.directoryImages(
        receiver, directoryUrl, std::move(callback), std::move(errorCallback));
}

ImageIoJob ImageCandidateRepository::loadArchiveImages(QObject *receiver,
    const QUrl &archiveRootUrl, ImageCandidatesCallback callback, ErrorCallback errorCallback) const
{
    if (!m_provider.archiveImages) {
        if (errorCallback) {
            errorCallback(QString());
        }
        return ImageIoJob();
    }

    return m_provider.archiveImages(
        receiver, archiveRootUrl, std::move(callback), std::move(errorCallback));
}

ImageIoJob ImageCandidateRepository::loadContainers(QObject *receiver, const QUrl &directoryUrl,
    ContainerCandidatesCallback callback, ErrorCallback errorCallback) const
{
    if (!m_provider.directoryContainers) {
        if (errorCallback) {
            errorCallback(QString());
        }
        return ImageIoJob();
    }

    return m_provider.directoryContainers(
        receiver, directoryUrl, std::move(callback), std::move(errorCallback));
}

ImageIoJob ImageCandidateRepository::loadFirstImageInContainer(QObject *receiver,
    const ContainerNavigationCandidate &container, ContainerImageCallback callback,
    CandidateRepositoryErrorCallback errorCallback) const
{
    if (container.type == ContainerNavigationCandidateType::ComicBookArchive) {
        const std::optional<QUrl> archiveRootUrl = comicBookArchiveRootUrl(container.url);
        if (!archiveRootUrl.has_value()) {
            if (errorCallback) {
                errorCallback(container.url, ImageCandidateRepositoryError::InvalidComicBookArchive,
                    QString());
            }
            return ImageIoJob();
        }

        return loadArchiveImages(
            receiver, archiveRootUrl.value(),
            [containerUrl = container.url, callback = std::move(callback), errorCallback](
                std::vector<ImageNavigationCandidate> candidates) mutable {
                if (candidates.empty()) {
                    if (errorCallback) {
                        errorCallback(
                            containerUrl, ImageCandidateRepositoryError::EmptyContainer, QString());
                    }
                    return;
                }

                if (callback) {
                    callback(candidates.front().url, containerUrl);
                }
            },
            [containerUrl = container.url, errorCallback](const QString &errorString) {
                if (errorCallback) {
                    errorCallback(
                        containerUrl, ImageCandidateRepositoryError::Generic, errorString);
                }
            });
    }

    return loadDirectoryImages(
        receiver, container.url,
        [containerUrl = container.url, callback = std::move(callback), errorCallback](
            std::vector<ImageNavigationCandidate> candidates) mutable {
            if (candidates.empty()) {
                if (errorCallback) {
                    errorCallback(
                        containerUrl, ImageCandidateRepositoryError::EmptyContainer, QString());
                }
                return;
            }

            if (callback) {
                callback(candidates.front().url, containerUrl);
            }
        },
        [containerUrl = container.url, errorCallback](const QString &errorString) {
            if (errorCallback) {
                errorCallback(containerUrl, ImageCandidateRepositoryError::Generic, errorString);
            }
        });
}
}
