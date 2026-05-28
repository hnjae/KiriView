// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpagecandidaterepository.h"

#include "async/imagecallback.h"

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

KiriView::ImageIoJob loadImagesForSource(
    const KiriView::ImageDocumentPageCandidateRepository &repository, QObject *receiver,
    const KiriView::ImageDocumentPageCandidateListSource::Directory &source,
    KiriView::ImageDocumentPageCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
{
    return repository.loadDirectoryImages(
        receiver, source.directoryUrl, std::move(callback), std::move(errorCallback));
}

KiriView::ImageIoJob loadImagesForSource(
    const KiriView::ImageDocumentPageCandidateRepository &repository, QObject *receiver,
    const KiriView::ImageDocumentPageCandidateListSource::OpenedCollectionScope &source,
    KiriView::ImageDocumentPageCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
{
    return repository.loadOpenedCollectionCandidates(
        receiver, source.openedCollectionScope, std::move(callback), std::move(errorCallback));
}

KiriView::ImageIoJob watchChangesForSource(
    const KiriView::ImageDocumentPageCandidateRepository &repository, QObject *receiver,
    const KiriView::ImageDocumentPageCandidateListSource::Directory &source,
    KiriView::ImageDocumentPageCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
{
    return repository.watchDirectoryImageChanges(
        receiver, source.directoryUrl, std::move(callback), std::move(errorCallback));
}

KiriView::ImageIoJob watchChangesForSource(const KiriView::ImageDocumentPageCandidateRepository &,
    QObject *, const KiriView::ImageDocumentPageCandidateListSource::OpenedCollectionScope &,
    KiriView::ImageDocumentPageCandidatesCallback, KiriView::ErrorCallback)
{
    return KiriView::ImageIoJob();
}

}

namespace KiriView {
ImageDocumentPageCandidateRepository::ImageDocumentPageCandidateRepository(
    ImageDocumentPageCandidateProvider provider)
    : m_provider(imageDocumentPageNavigationCandidateProviderWithDefaults(std::move(provider)))
{
}

ImageIoJob ImageDocumentPageCandidateRepository::loadImages(QObject *receiver,
    const ImageDocumentPageCandidateListSource &source,
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) const
{
    return source.visit(
        [this, receiver, callback = std::move(callback), errorCallback = std::move(errorCallback)](
            const auto &typedSource) mutable {
            return loadImagesForSource(
                *this, receiver, typedSource, std::move(callback), std::move(errorCallback));
        });
}

ImageIoJob ImageDocumentPageCandidateRepository::loadImages(QObject *receiver,
    const ImageDocumentPageCandidateListContext &context,
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) const
{
    return loadImages(receiver, context.source(), std::move(callback), std::move(errorCallback));
}

ImageIoJob ImageDocumentPageCandidateRepository::loadDirectoryImages(QObject *receiver,
    const QUrl &directoryUrl, ImageDocumentPageCandidatesCallback callback,
    ErrorCallback errorCallback) const
{
    return loadWithProvider(m_provider.directoryImageDocumentPages, std::move(errorCallback),
        receiver, directoryUrl, std::move(callback));
}

ImageIoJob ImageDocumentPageCandidateRepository::loadOpenedCollectionCandidates(QObject *receiver,
    OpenedCollectionScopeLocation openedCollectionScope,
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) const
{
    return loadWithProvider(m_provider.openedCollectionCandidates, std::move(errorCallback),
        receiver, std::move(openedCollectionScope), std::move(callback));
}

ImageIoJob ImageDocumentPageCandidateRepository::loadContainers(QObject *receiver,
    const QUrl &directoryUrl, ContainerCandidatesCallback callback,
    ErrorCallback errorCallback) const
{
    return loadWithProvider(m_provider.directoryContainers, std::move(errorCallback), receiver,
        directoryUrl, std::move(callback));
}

ImageIoJob ImageDocumentPageCandidateRepository::watchCandidateChanges(QObject *receiver,
    const ImageDocumentPageCandidateListSource &source,
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) const
{
    return source.visit(
        [this, receiver, callback = std::move(callback), errorCallback = std::move(errorCallback)](
            const auto &typedSource) mutable {
            return watchChangesForSource(
                *this, receiver, typedSource, std::move(callback), std::move(errorCallback));
        });
}

ImageIoJob ImageDocumentPageCandidateRepository::watchCandidateChanges(QObject *receiver,
    const ImageDocumentPageCandidateListContext &context,
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) const
{
    return watchCandidateChanges(
        receiver, context.source(), std::move(callback), std::move(errorCallback));
}

ImageIoJob ImageDocumentPageCandidateRepository::watchDirectoryImageChanges(QObject *receiver,
    const QUrl &directoryUrl, ImageDocumentPageCandidatesCallback callback,
    ErrorCallback errorCallback) const
{
    return loadWithProvider(m_provider.directoryImageDocumentPageChanges, std::move(errorCallback),
        receiver, directoryUrl, std::move(callback));
}
}
