// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaentrysourcestore.h"

#include "async/imagecallback.h"
#include "document/imagedocumentsourceloadrequest.h"
#include "document/imageloadplan.h"
#include "location/imagedocumentlocation.h"

#include <optional>
#include <utility>

namespace {
std::optional<KiriView::OpenedCollectionScopeLocation> openedCollectionScopeForSourceLoad(
    const KiriView::ImageDocumentSourceLoadRequest &request,
    const KiriView::OpenedCollectionScopeLocation &displayedOpenedCollectionScope)
{
    const KiriView::OpenedCollectionScopeLoadPlan plan
        = KiriView::openedCollectionScopeLoadPlan(KiriView::ImageLoadRequest::fromLocation(
            request.sourceUrl, displayedOpenedCollectionScope, request.containerNavigationUrl));
    if (plan.openedCollectionScope.isEmpty()) {
        return std::nullopt;
    }

    return plan.openedCollectionScope;
}
}

namespace KiriView {
MediaEntrySourceStore::MediaEntrySourceStore(MediaEntrySourceFactory sourceFactory, QObject *parent)
    : QObject(parent)
    , m_runtime(this, std::move(sourceFactory))
{
}

MediaEntrySourceStore::~MediaEntrySourceStore() { clear(); }

ImageNavigationCandidateProvider MediaEntrySourceStore::wrapCandidateProvider(
    ImageNavigationCandidateProvider provider)
{
    provider.openedCollectionCandidates
        = [this](QObject *receiver, OpenedCollectionScopeLocation archiveCollection,
              ImageCandidatesCallback callback, ErrorCallback errorCallback) {
              return loadOpenedCollectionCandidates(receiver, std::move(archiveCollection),
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

void MediaEntrySourceStore::prepareForSourceLoad(const ImageDocumentSourceLoadRequest &request,
    const OpenedCollectionScopeLocation &displayedOpenedCollectionScope)
{
    const std::optional<OpenedCollectionScopeLocation> archiveCollection
        = openedCollectionScopeForSourceLoad(request, displayedOpenedCollectionScope);
    if (archiveCollection.has_value()) {
        m_runtime.switchToOpenedCollectionScope(*archiveCollection);
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
    const OpenedCollectionScopeLocation &archiveCollection) const
{
    return m_runtime.hasCurrentOpenedCollectionScope(archiveCollection);
}

ImageIoJob MediaEntrySourceStore::loadOpenedCollectionCandidates(QObject *receiver,
    OpenedCollectionScopeLocation archiveCollection, ImageCandidatesCallback callback,
    ErrorCallback errorCallback)
{
    return m_runtime.loadOpenedCollectionCandidates(
        receiver, std::move(archiveCollection), std::move(callback), std::move(errorCallback));
}

ImageIoJob MediaEntrySourceStore::loadOpenedCollectionImageData(QObject *receiver,
    ImageDecodeRequest request, ImageDataCallback callback, ErrorCallback errorCallback)
{
    return m_runtime.loadOpenedCollectionImageData(
        receiver, std::move(request), std::move(callback), std::move(errorCallback));
}
}
