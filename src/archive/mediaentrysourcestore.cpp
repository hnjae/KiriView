// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaentrysourcestore.h"

#include "async/imagecallback.h"
#include "location/imagedocumentlocation.h"

#include <utility>

namespace kiriview {
MediaEntrySourceStore::MediaEntrySourceStore(
    MediaEntrySourceFactory sourceFactory, QObject *parent, ImageWorkerScheduler workerScheduler)
    : QObject(parent)
    , m_runtime(this, std::move(sourceFactory), std::move(workerScheduler))
{
}

MediaEntrySourceStore::~MediaEntrySourceStore() { clear(); }

ImageDocumentPageCandidateProvider MediaEntrySourceStore::wrapCandidateProvider(
    ImageDocumentPageCandidateProvider provider)
{
    provider.openedCollectionCandidates
        = [this](QObject *receiver, OpenedCollectionScopeLocation openedCollectionScope,
              ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) {
              return loadOpenedCollectionCandidates(receiver, std::move(openedCollectionScope),
                  std::move(callback), std::move(errorCallback));
          };
    return provider;
}

ImageDecodeDependencies MediaEntrySourceStore::wrapDecodeDependencies(
    ImageDecodeDependencies dependencies)
{
    ImageDataLoader upstreamDataLoader = std::move(dependencies.dataLoader);
    dependencies.dataLoader = [this, upstreamDataLoader = std::move(upstreamDataLoader)](
                                  QObject *receiver, ImageDecodeRequest request,
                                  ImageDataCallback callback, ErrorCallback errorCallback) {
        if (openedCollectionScopeContainsUrl(request.openedCollectionScope(), request.imageUrl())) {
            return loadOpenedCollectionImageData(
                receiver, std::move(request), std::move(callback), std::move(errorCallback));
        }

        if (!upstreamDataLoader) {
            invokeIfSet(errorCallback, QString());
            return ImageIoJob();
        }

        return upstreamDataLoader(
            receiver, std::move(request), std::move(callback), std::move(errorCallback));
    };
    return dependencies;
}

void MediaEntrySourceStore::prepareForOpenedCollectionScope(
    const OpenedCollectionScopeLocation &openedCollectionScope)
{
    if (!openedCollectionScope.isEmpty()) {
        m_runtime.switchToOpenedCollectionScope(openedCollectionScope);
        return;
    }

    clear();
}

void MediaEntrySourceStore::clear() { m_runtime.clear(); }

bool MediaEntrySourceStore::hasCurrentOpenedCollectionScope() const
{
    return m_runtime.hasCurrentOpenedCollectionScope();
}

bool MediaEntrySourceStore::hasCurrentOpenedCollectionScope(
    const OpenedCollectionScopeLocation &openedCollectionScope) const
{
    return m_runtime.hasCurrentOpenedCollectionScope(openedCollectionScope);
}

ImageIoJob MediaEntrySourceStore::loadOpenedCollectionCandidates(QObject *receiver,
    OpenedCollectionScopeLocation openedCollectionScope,
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback)
{
    return m_runtime.loadOpenedCollectionCandidates(
        receiver, std::move(openedCollectionScope), std::move(callback), std::move(errorCallback));
}

ImageIoJob MediaEntrySourceStore::loadOpenedCollectionImageData(QObject *receiver,
    ImageDecodeRequest request, ImageDataCallback callback, ErrorCallback errorCallback)
{
    return m_runtime.loadOpenedCollectionImageData(
        receiver, std::move(request), std::move(callback), std::move(errorCallback));
}
}
