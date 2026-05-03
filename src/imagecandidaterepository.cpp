// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidaterepository.h"

#include "imagecontainer.h"
#include "imageiojobs.h"
#include "imageurl.h"

#include <utility>
#include <vector>

namespace {
void reportCandidateRepositoryError(const QUrl &containerUrl,
    KiriView::ImageCandidateRepositoryError error, const QString &errorString,
    const KiriView::CandidateRepositoryErrorCallback &errorCallback)
{
    if (errorCallback) {
        errorCallback(containerUrl, error, errorString);
    }
}

struct FirstContainerImageSelector {
    QUrl containerUrl;
    KiriView::ContainerImageCallback imageCallback;
    KiriView::CandidateRepositoryErrorCallback errorCallback;

    void operator()(std::vector<KiriView::ImageNavigationCandidate> candidates)
    {
        if (candidates.empty()) {
            reportCandidateRepositoryError(containerUrl,
                KiriView::ImageCandidateRepositoryError::EmptyContainer, QString(), errorCallback);
            return;
        }

        if (imageCallback) {
            imageCallback(candidates.front().url, containerUrl);
        }
    }
};
}

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
    const DisplayedImageLocation &location)
{
    const QUrl &displayedUrl = location.imageUrl();
    if (displayedUrl.isEmpty()) {
        return std::nullopt;
    }

    if (!location.archiveDocument().isEmpty()
        && isUrlInsideArchiveRoot(displayedUrl, location.archiveDocumentRootUrl())) {
        const QUrl currentUrl = normalizedImageUrl(displayedUrl);
        if (!currentUrl.isValid()) {
            return std::nullopt;
        }

        return ImageCandidateListContext {
            currentUrl,
            location.archiveDocumentRootUrl(),
            location.archiveDocument(),
            ImageCandidateContainerType::ArchiveDocument,
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
        ArchiveDocumentLocation::none(),
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
    if (context.containerType == ImageCandidateContainerType::ArchiveDocument) {
        return loadArchiveImages(
            receiver, context.archiveDocument, std::move(callback), std::move(errorCallback));
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
    ArchiveDocumentLocation archiveDocument, ImageCandidatesCallback callback,
    ErrorCallback errorCallback) const
{
    if (!m_provider.archiveImages) {
        if (errorCallback) {
            errorCallback(QString());
        }
        return ImageIoJob();
    }

    return m_provider.archiveImages(
        receiver, std::move(archiveDocument), std::move(callback), std::move(errorCallback));
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
    CandidateRepositoryErrorCallback sharedErrorCallback = std::move(errorCallback);
    ImageCandidatesCallback firstImageCallback = FirstContainerImageSelector {
        container.url,
        std::move(callback),
        sharedErrorCallback,
    };
    return loadImagesInContainer(
        receiver, container, std::move(firstImageCallback), std::move(sharedErrorCallback));
}

ImageIoJob ImageCandidateRepository::loadImagesInContainer(QObject *receiver,
    const ContainerNavigationCandidate &container, ImageCandidatesCallback callback,
    CandidateRepositoryErrorCallback errorCallback) const
{
    if (container.type == ContainerNavigationCandidateType::ComicBookArchive) {
        const std::optional<ArchiveDocumentLocation> archiveDocument
            = archiveDocumentLocationForLocalArchiveUrl(container.url);
        if (!archiveDocument.has_value() || !archiveDocument->isComicBook()) {
            reportCandidateRepositoryError(container.url,
                ImageCandidateRepositoryError::InvalidComicBookArchive, QString(), errorCallback);
            return ImageIoJob();
        }

        return loadArchiveImages(receiver, *archiveDocument, std::move(callback),
            [containerUrl = container.url, errorCallback](const QString &errorString) {
                reportCandidateRepositoryError(containerUrl, ImageCandidateRepositoryError::Generic,
                    errorString, errorCallback);
            });
    }

    return loadDirectoryImages(receiver, container.url, std::move(callback),
        [containerUrl = container.url, errorCallback](const QString &errorString) {
            reportCandidateRepositoryError(
                containerUrl, ImageCandidateRepositoryError::Generic, errorString, errorCallback);
        });
}
}
