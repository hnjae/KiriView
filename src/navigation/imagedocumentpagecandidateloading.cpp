// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagedocumentpagecandidateloading.h"

#include "async/directorylistingjob.h"
#include "async/imagecallback.h"
#include "navigation/imagedocumentpagecandidateitems.h"

#include <utility>

namespace {
template <typename CandidateCallback, typename CandidateFactory>
kiriview::ImageIoJob startDirectoryCandidateList(QObject* receiver, const QUrl& directoryUrl,
    CandidateCallback callback, kiriview::ErrorCallback errorCallback,
    kiriview::DirectoryItemListProvider directoryItemListProvider,
    CandidateFactory candidateFactory)
{
    return kiriview::startDirectoryItemList(
        receiver, directoryUrl,
        [callback = std::move(callback), candidateFactory = std::move(candidateFactory)](
            KFileItemList items) mutable {
            kiriview::invokeIfSet(callback, candidateFactory(items));
        },
        std::move(errorCallback), std::move(directoryItemListProvider));
}
}

namespace kiriview {
ImageIoJob startDirectoryImageDocumentPageCandidateList(QObject* receiver, QUrl directoryUrl,
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback)
{
    return startDirectoryCandidateList(receiver, directoryUrl, std::move(callback),
        std::move(errorCallback), {}, imageDocumentPageNavigationCandidates);
}

ImageIoJob startDirectoryImageDocumentPageCandidateList(QObject* receiver, QUrl directoryUrl,
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback,
    DirectoryItemListProvider directoryItemListProvider)
{
    return startDirectoryCandidateList(receiver, directoryUrl, std::move(callback),
        std::move(errorCallback), std::move(directoryItemListProvider),
        imageDocumentPageNavigationCandidates);
}

ImageIoJob startDirectoryContainerCandidateList(QObject* receiver, QUrl directoryUrl,
    ContainerCandidatesCallback callback, ErrorCallback errorCallback)
{
    return startDirectoryCandidateList(receiver, directoryUrl, std::move(callback),
        std::move(errorCallback), {}, containerNavigationCandidates);
}

ImageIoJob startDirectoryContainerCandidateList(QObject* receiver, QUrl directoryUrl,
    ContainerCandidatesCallback callback, ErrorCallback errorCallback,
    DirectoryItemListProvider directoryItemListProvider)
{
    return startDirectoryCandidateList(receiver, directoryUrl, std::move(callback),
        std::move(errorCallback), std::move(directoryItemListProvider),
        containerNavigationCandidates);
}
}
