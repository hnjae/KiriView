// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidaterepository.h"

#include "imagecallback.h"
#include "imagecandidatestore.h"
#include "imagecontainer.h"
#include "imagecontaineropenplan.h"
#include "imageiojobs.h"

#include <memory>
#include <utility>
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

KiriView::ImageIoJob watchChangesForSource(const KiriView::ImageCandidateRepository &repository,
    QObject *receiver, const KiriView::ImageCandidateListSource::Directory &source,
    KiriView::ImageCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
{
    return repository.watchDirectoryImageChanges(
        receiver, source.directoryUrl, std::move(callback), std::move(errorCallback));
}

KiriView::ImageIoJob watchChangesForSource(const KiriView::ImageCandidateRepository &, QObject *,
    const KiriView::ImageCandidateListSource::ArchiveDocument &, KiriView::ImageCandidatesCallback,
    KiriView::ErrorCallback)
{
    return KiriView::ImageIoJob();
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

}

namespace KiriView {
ImageNavigationCandidateProvider defaultImageNavigationCandidateProvider()
{
    auto candidateStore = std::make_shared<ImageCandidateStore>();
    return ImageNavigationCandidateProvider {
        [candidateStore](QObject *receiver, QUrl directoryUrl, ImageCandidatesCallback callback,
            ErrorCallback errorCallback) {
            return candidateStore->loadDirectoryImages(
                receiver, std::move(directoryUrl), std::move(callback), std::move(errorCallback));
        },
        startDirectoryContainerCandidateList,
        startArchiveImageCandidateList,
        [candidateStore](QObject *receiver, QUrl directoryUrl, ImageCandidatesCallback callback,
            ErrorCallback errorCallback) {
            return candidateStore->watchDirectoryImages(
                receiver, std::move(directoryUrl), std::move(callback), std::move(errorCallback));
        },
    };
}

ImageCandidateRepository::ImageCandidateRepository(ImageNavigationCandidateProvider provider)
    : m_provider(imageNavigationCandidateProviderWithDefaults(std::move(provider)))
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

ImageIoJob ImageCandidateRepository::watchCandidateChanges(QObject *receiver,
    const ImageCandidateListSource &source, ImageCandidatesCallback callback,
    ErrorCallback errorCallback) const
{
    return source.visit(
        [this, receiver, callback = std::move(callback), errorCallback = std::move(errorCallback)](
            const auto &typedSource) mutable {
            return watchChangesForSource(
                *this, receiver, typedSource, std::move(callback), std::move(errorCallback));
        });
}

ImageIoJob ImageCandidateRepository::watchCandidateChanges(QObject *receiver,
    const ImageCandidateListContext &context, ImageCandidatesCallback callback,
    ErrorCallback errorCallback) const
{
    return watchCandidateChanges(
        receiver, context.source(), std::move(callback), std::move(errorCallback));
}

ImageIoJob ImageCandidateRepository::watchDirectoryImageChanges(QObject *receiver,
    const QUrl &directoryUrl, ImageCandidatesCallback callback, ErrorCallback errorCallback) const
{
    return loadWithProvider(m_provider.directoryImageChanges, std::move(errorCallback), receiver,
        directoryUrl, std::move(callback));
}

ImageIoJob ImageCandidateRepository::loadFirstImageInContainer(QObject *receiver,
    const ContainerNavigationCandidate &container, ContainerImageCallback callback,
    CandidateRepositoryErrorCallback errorCallback) const
{
    CandidateRepositoryErrorCallback sharedErrorCallback = std::move(errorCallback);
    ImageCandidatesCallback firstImageCallback
        = [containerUrl = container.url, imageCallback = std::move(callback),
              errorCallback = sharedErrorCallback](
              const std::vector<ImageNavigationCandidate> &candidates) {
              const ImageContainerOpenResult result
                  = imageContainerOpenResultForCandidates(candidates);
              if (result.openedImage()) {
                  invokeIfSet(imageCallback, *result.imageUrl, containerUrl);
                  return;
              }

              reportCandidateRepositoryError(containerUrl, result.error, QString(), errorCallback);
          };
    return loadImagesInContainer(
        receiver, container, std::move(firstImageCallback), std::move(sharedErrorCallback));
}

ImageIoJob ImageCandidateRepository::loadImagesInContainer(QObject *receiver,
    const ContainerNavigationCandidate &container, ImageCandidatesCallback callback,
    CandidateRepositoryErrorCallback errorCallback) const
{
    ImageContainerOpenPlan plan = imageContainerOpenPlanForCandidate(container);
    if (!plan.shouldLoadCandidates()) {
        reportCandidateRepositoryError(container.url, plan.error, QString(), errorCallback);
        return ImageIoJob();
    }

    return loadImages(receiver, *plan.source, std::move(callback),
        containerLoadErrorCallback(container.url, std::move(errorCallback)));
}
}
