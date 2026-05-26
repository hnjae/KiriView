// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivedocumentsessionstore.h"

#include "async/imagecallback.h"
#include "document/imagedocumentsourceloadrequest.h"
#include "document/imageloadplan.h"
#include "location/imagedocumentlocation.h"

#include <optional>
#include <utility>

namespace {
std::optional<KiriView::ImagePageScopeLocation> archiveDocumentForSourceLoad(
    const KiriView::ImageDocumentSourceLoadRequest &request,
    const KiriView::ImagePageScopeLocation &displayedImagePageScope)
{
    const KiriView::ImagePageScopeLoadPlan plan
        = KiriView::imagePageScopeLoadPlan(KiriView::ImageLoadRequest::fromLocation(
            request.sourceUrl, displayedImagePageScope, request.containerNavigationUrl));
    if (plan.imagePageScope.isEmpty()) {
        return std::nullopt;
    }

    return plan.imagePageScope;
}
}

namespace KiriView {
ArchiveDocumentSessionStore::ArchiveDocumentSessionStore(
    ArchiveDocumentSessionFactory sessionFactory, QObject *parent)
    : QObject(parent)
    , m_runtime(this, std::move(sessionFactory))
{
}

ArchiveDocumentSessionStore::~ArchiveDocumentSessionStore() { clear(); }

ImageNavigationCandidateProvider ArchiveDocumentSessionStore::wrapCandidateProvider(
    ImageNavigationCandidateProvider provider)
{
    provider.archiveImages = [this](QObject *receiver, ImagePageScopeLocation archiveDocument,
                                 ImageCandidatesCallback callback, ErrorCallback errorCallback) {
        return loadArchiveImages(
            receiver, std::move(archiveDocument), std::move(callback), std::move(errorCallback));
    };
    return provider;
}

ImageDecodeDependencies ArchiveDocumentSessionStore::wrapDecodeDependencies(
    ImageDecodeDependencies dependencies)
{
    ImageDataLoader upstreamDataLoader = std::move(dependencies.dataLoader);
    dependencies.dataLoader
        = [this, upstreamDataLoader = std::move(upstreamDataLoader)](QObject *receiver,
              ImageDecodeRequest request, ImageDataCallback callback, ErrorCallback errorCallback) {
              if (imagePageScopeContainsUrl(request.imagePageScope(), request.imageUrl())) {
                  return loadArchiveImageData(
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

void ArchiveDocumentSessionStore::prepareForSourceLoad(
    const ImageDocumentSourceLoadRequest &request,
    const ImagePageScopeLocation &displayedImagePageScope)
{
    const std::optional<ImagePageScopeLocation> archiveDocument
        = archiveDocumentForSourceLoad(request, displayedImagePageScope);
    if (archiveDocument.has_value()) {
        m_runtime.switchToArchiveDocument(*archiveDocument);
        return;
    }

    clear();
}

void ArchiveDocumentSessionStore::clear() { m_runtime.clear(); }

bool ArchiveDocumentSessionStore::hasCurrentArchiveDocument() const
{
    return m_runtime.hasCurrentArchiveDocument();
}

bool ArchiveDocumentSessionStore::hasCurrentArchiveDocument(
    const ImagePageScopeLocation &archiveDocument) const
{
    return m_runtime.hasCurrentArchiveDocument(archiveDocument);
}

ImageIoJob ArchiveDocumentSessionStore::loadArchiveImages(QObject *receiver,
    ImagePageScopeLocation archiveDocument, ImageCandidatesCallback callback,
    ErrorCallback errorCallback)
{
    return m_runtime.loadArchiveImages(
        receiver, std::move(archiveDocument), std::move(callback), std::move(errorCallback));
}

ImageIoJob ArchiveDocumentSessionStore::loadArchiveImageData(QObject *receiver,
    ImageDecodeRequest request, ImageDataCallback callback, ErrorCallback errorCallback)
{
    return m_runtime.loadArchiveImageData(
        receiver, std::move(request), std::move(callback), std::move(errorCallback));
}
}
