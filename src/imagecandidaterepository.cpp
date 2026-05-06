// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidaterepository.h"

#include "imagecallback.h"
#include "imagecontainer.h"
#include "imageiojobs.h"
#include "imageurl.h"

#include <utility>
#include <variant>
#include <vector>

namespace {
void reportLoadProviderMissing(const KiriView::ErrorCallback &errorCallback)
{
    KiriView::invokeIfSet(errorCallback, QString());
}

template <typename Provider, typename... Args>
KiriView::ImageIoJob loadWithProvider(
    const Provider &provider, KiriView::ErrorCallback errorCallback, Args &&...args)
{
    if (!provider) {
        reportLoadProviderMissing(errorCallback);
        return KiriView::ImageIoJob();
    }

    return provider(std::forward<Args>(args)..., std::move(errorCallback));
}

KiriView::ImageIoJob loadImagesForContext(const KiriView::ImageCandidateRepository &repository,
    QObject *receiver, const KiriView::ImageCandidateListContext::DirectoryContext &context,
    KiriView::ImageCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
{
    return repository.loadDirectoryImages(
        receiver, context.directoryUrl, std::move(callback), std::move(errorCallback));
}

KiriView::ImageIoJob loadImagesForContext(const KiriView::ImageCandidateRepository &repository,
    QObject *receiver, const KiriView::ImageCandidateListContext::ArchiveDocumentContext &context,
    KiriView::ImageCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
{
    return repository.loadArchiveImages(
        receiver, context.archiveDocument, std::move(callback), std::move(errorCallback));
}

void reportCandidateRepositoryError(const QUrl &containerUrl,
    KiriView::ImageCandidateRepositoryError error, const QString &errorString,
    const KiriView::CandidateRepositoryErrorCallback &errorCallback)
{
    KiriView::invokeIfSet(errorCallback, containerUrl, error, errorString);
}

KiriView::ErrorCallback containerLoadErrorCallback(
    QUrl containerUrl, KiriView::CandidateRepositoryErrorCallback errorCallback)
{
    return [containerUrl = std::move(containerUrl), errorCallback = std::move(errorCallback)](
               const QString &errorString) {
        reportCandidateRepositoryError(containerUrl,
            KiriView::ImageCandidateRepositoryError::Generic, errorString, errorCallback);
    };
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

        KiriView::invokeIfSet(imageCallback, candidates.front().url, containerUrl);
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

ImageCandidateListContext ImageCandidateListContext::forDirectory(
    QUrl currentUrl, QUrl directoryUrl)
{
    return ImageCandidateListContext {
        DirectoryContext { std::move(currentUrl), std::move(directoryUrl) },
    };
}

ImageCandidateListContext ImageCandidateListContext::forArchiveDocument(
    QUrl currentUrl, ArchiveDocumentLocation archiveDocument)
{
    return ImageCandidateListContext {
        ArchiveDocumentContext { std::move(currentUrl), std::move(archiveDocument) },
    };
}

const QUrl &ImageCandidateListContext::currentUrl() const
{
    return visit([](const auto &context) -> const QUrl & { return context.currentUrl; });
}

ArchiveDocumentLocation ImageCandidateListContext::archiveDocument() const
{
    const auto *archiveContext = std::get_if<ArchiveDocumentContext>(&m_context);
    return archiveContext == nullptr ? ArchiveDocumentLocation::none()
                                     : archiveContext->archiveDocument;
}

ImageCandidateListContext::ImageCandidateListContext(Context context)
    : m_context(std::move(context))
{
}

std::optional<ImageCandidateListContext> imageCandidateListContextForDisplayedImage(
    const DisplayedImageLocation &location)
{
    const QUrl &displayedUrl = location.imageUrl();
    if (displayedUrl.isEmpty()) {
        return std::nullopt;
    }

    if (displayedLocationIsInsideArchiveDocument(location)) {
        const QUrl currentUrl = normalizedImageUrl(displayedUrl);
        if (!currentUrl.isValid()) {
            return std::nullopt;
        }

        return ImageCandidateListContext::forArchiveDocument(
            currentUrl, location.archiveDocument());
    }

    const QUrl currentUrl = navigationSourceUrl(displayedUrl);
    const QUrl parentUrl = currentUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
    if (!currentUrl.isValid() || currentUrl.isEmpty() || !parentUrl.isValid()
        || parentUrl.isEmpty()) {
        return std::nullopt;
    }

    return ImageCandidateListContext::forDirectory(currentUrl, parentUrl);
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
    return context.visit(
        [this, receiver, callback = std::move(callback), errorCallback = std::move(errorCallback)](
            const auto &typedContext) mutable {
            return loadImagesForContext(
                *this, receiver, typedContext, std::move(callback), std::move(errorCallback));
        });
}

ImageIoJob ImageCandidateRepository::loadDirectoryImages(QObject *receiver,
    const QUrl &directoryUrl, ImageCandidatesCallback callback, ErrorCallback errorCallback) const
{
    return loadWithProvider(m_provider.directoryImages, std::move(errorCallback), receiver,
        directoryUrl, std::move(callback));
}

ImageIoJob ImageCandidateRepository::loadArchiveImages(QObject *receiver,
    ArchiveDocumentLocation archiveDocument, ImageCandidatesCallback callback,
    ErrorCallback errorCallback) const
{
    return loadWithProvider(m_provider.archiveImages, std::move(errorCallback), receiver,
        std::move(archiveDocument), std::move(callback));
}

ImageIoJob ImageCandidateRepository::loadContainers(QObject *receiver, const QUrl &directoryUrl,
    ContainerCandidatesCallback callback, ErrorCallback errorCallback) const
{
    return loadWithProvider(m_provider.directoryContainers, std::move(errorCallback), receiver,
        directoryUrl, std::move(callback));
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
    switch (container.type) {
    case ContainerNavigationCandidateType::Directory:
        return loadDirectoryImagesInContainer(
            receiver, container, std::move(callback), std::move(errorCallback));
    case ContainerNavigationCandidateType::ComicBookArchive:
        return loadComicBookArchiveImagesInContainer(
            receiver, container, std::move(callback), std::move(errorCallback));
    }

    reportCandidateRepositoryError(
        container.url, ImageCandidateRepositoryError::Generic, QString(), errorCallback);
    return ImageIoJob();
}

ImageIoJob ImageCandidateRepository::loadDirectoryImagesInContainer(QObject *receiver,
    const ContainerNavigationCandidate &container, ImageCandidatesCallback callback,
    CandidateRepositoryErrorCallback errorCallback) const
{
    return loadDirectoryImages(receiver, container.url, std::move(callback),
        containerLoadErrorCallback(container.url, std::move(errorCallback)));
}

ImageIoJob ImageCandidateRepository::loadComicBookArchiveImagesInContainer(QObject *receiver,
    const ContainerNavigationCandidate &container, ImageCandidatesCallback callback,
    CandidateRepositoryErrorCallback errorCallback) const
{
    const std::optional<ArchiveDocumentLocation> archiveDocument
        = archiveDocumentLocationForLocalArchiveUrl(container.url);
    if (!archiveDocument.has_value() || !archiveDocument->isComicBook()) {
        reportCandidateRepositoryError(container.url,
            ImageCandidateRepositoryError::InvalidComicBookArchive, QString(), errorCallback);
        return ImageIoJob();
    }

    return loadArchiveImages(receiver, *archiveDocument, std::move(callback),
        containerLoadErrorCallback(container.url, std::move(errorCallback)));
}
}
