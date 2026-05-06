// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidaterepository.h"

#include "imagecallback.h"
#include "imagecontainer.h"
#include "imageiojobs.h"
#include "imageurl.h"

#include <optional>
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

KiriView::ImageIoJob loadImagesForSource(const KiriView::ImageCandidateRepository &repository,
    QObject *receiver, const KiriView::ImageCandidateListSource::Directory &source,
    KiriView::ImageCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
{
    return repository.loadDirectoryImages(
        receiver, source.directoryUrl, std::move(callback), std::move(errorCallback));
}

KiriView::ImageIoJob loadImagesForSource(const KiriView::ImageCandidateRepository &repository,
    QObject *receiver, const KiriView::ImageCandidateListSource::ArchiveDocument &source,
    KiriView::ImageCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
{
    return repository.loadArchiveImages(
        receiver, source.archiveDocument, std::move(callback), std::move(errorCallback));
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

struct ContainerImageSourceResult {
    std::optional<KiriView::ImageCandidateListSource> source;
    KiriView::ImageCandidateRepositoryError error
        = KiriView::ImageCandidateRepositoryError::Generic;
};

ContainerImageSourceResult containerImageSourceFor(
    const KiriView::ContainerNavigationCandidate &container)
{
    switch (container.type) {
    case KiriView::ContainerNavigationCandidateType::Directory:
        return { KiriView::ImageCandidateListSource::forDirectory(container.url),
            KiriView::ImageCandidateRepositoryError::Generic };
    case KiriView::ContainerNavigationCandidateType::ComicBookArchive: {
        const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
            = KiriView::archiveDocumentLocationForLocalArchiveUrl(container.url);
        if (!archiveDocument.has_value() || !archiveDocument->isComicBook()) {
            return { std::nullopt,
                KiriView::ImageCandidateRepositoryError::InvalidComicBookArchive };
        }

        return { KiriView::ImageCandidateListSource::forArchiveDocument(*archiveDocument),
            KiriView::ImageCandidateRepositoryError::Generic };
    }
    }

    return { std::nullopt, KiriView::ImageCandidateRepositoryError::Generic };
}
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

ImageCandidateListSource ImageCandidateListSource::forDirectory(QUrl directoryUrl)
{
    return ImageCandidateListSource { Directory { std::move(directoryUrl) } };
}

ImageCandidateListSource ImageCandidateListSource::forArchiveDocument(
    ArchiveDocumentLocation archiveDocument)
{
    return ImageCandidateListSource { ArchiveDocument { std::move(archiveDocument) } };
}

ArchiveDocumentLocation ImageCandidateListSource::archiveDocument() const
{
    const auto *archiveSource = std::get_if<ArchiveDocument>(&m_source);
    return archiveSource == nullptr ? ArchiveDocumentLocation::none()
                                    : archiveSource->archiveDocument;
}

ImageCandidateListSource::ImageCandidateListSource(Payload source)
    : m_source(std::move(source))
{
}

ImageCandidateListContext ImageCandidateListContext::forDirectory(
    QUrl currentUrl, QUrl directoryUrl)
{
    return ImageCandidateListContext {
        std::move(currentUrl),
        ImageCandidateListSource::forDirectory(std::move(directoryUrl)),
    };
}

ImageCandidateListContext ImageCandidateListContext::forArchiveDocument(
    QUrl currentUrl, ArchiveDocumentLocation archiveDocument)
{
    return ImageCandidateListContext {
        std::move(currentUrl),
        ImageCandidateListSource::forArchiveDocument(std::move(archiveDocument)),
    };
}

const QUrl &ImageCandidateListContext::currentUrl() const { return m_currentUrl; }

const ImageCandidateListSource &ImageCandidateListContext::source() const { return m_source; }

ArchiveDocumentLocation ImageCandidateListContext::archiveDocument() const
{
    return m_source.archiveDocument();
}

ImageCandidateListContext::ImageCandidateListContext(
    QUrl currentUrl, ImageCandidateListSource source)
    : m_currentUrl(std::move(currentUrl))
    , m_source(std::move(source))
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
    const ImageCandidateListSource &source, ImageCandidatesCallback callback,
    ErrorCallback errorCallback) const
{
    return source.visit(
        [this, receiver, callback = std::move(callback), errorCallback = std::move(errorCallback)](
            const auto &typedSource) mutable {
            return loadImagesForSource(
                *this, receiver, typedSource, std::move(callback), std::move(errorCallback));
        });
}

ImageIoJob ImageCandidateRepository::loadImages(QObject *receiver,
    const ImageCandidateListContext &context, ImageCandidatesCallback callback,
    ErrorCallback errorCallback) const
{
    return loadImages(receiver, context.source(), std::move(callback), std::move(errorCallback));
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
    ContainerImageSourceResult sourceResult = containerImageSourceFor(container);
    if (!sourceResult.source.has_value()) {
        reportCandidateRepositoryError(container.url, sourceResult.error, QString(), errorCallback);
        return ImageIoJob();
    }

    return loadImages(receiver, *sourceResult.source, std::move(callback),
        containerLoadErrorCallback(container.url, std::move(errorCallback)));
}
}
