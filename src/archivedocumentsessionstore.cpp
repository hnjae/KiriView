// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivedocumentsessionstore.h"

#include "imagecallback.h"
#include "imagecontainer.h"
#include "imagedocumentsourceloadrequest.h"
#include "imageloadplan.h"

#include <optional>
#include <utility>

namespace {
std::optional<KiriView::ArchiveDocumentLocation> archiveDocumentForSourceLoad(
    const KiriView::ImageDocumentSourceLoadRequest &request,
    const KiriView::ArchiveDocumentLocation &displayedArchiveDocument)
{
    const KiriView::ImageArchiveLoadPlan plan
        = KiriView::imageArchiveLoadPlan(KiriView::ImageLoadRequest::fromLocation(
            request.sourceUrl, displayedArchiveDocument, request.containerNavigationUrl));
    if (plan.archiveDocument.isEmpty()) {
        return std::nullopt;
    }

    return plan.archiveDocument;
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
    provider.archiveImages = [this](QObject *receiver, ArchiveDocumentLocation archiveDocument,
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
              if (archiveDocumentContainsUrl(request.archiveDocument(), request.imageUrl())) {
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

ImageAsyncDependencies ArchiveDocumentSessionStore::wrapDependencies(
    ImageAsyncDependencies dependencies)
{
    dependencies.candidateProvider
        = wrapCandidateProvider(std::move(dependencies.candidateProvider));
    dependencies.imageDecode = wrapDecodeDependencies(std::move(dependencies.imageDecode));
    return dependencies;
}

void ArchiveDocumentSessionStore::prepareForSourceLoad(
    const ImageDocumentSourceLoadRequest &request,
    const ArchiveDocumentLocation &displayedArchiveDocument)
{
    const std::optional<ArchiveDocumentLocation> archiveDocument
        = archiveDocumentForSourceLoad(request, displayedArchiveDocument);
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
    const ArchiveDocumentLocation &archiveDocument) const
{
    return m_runtime.hasCurrentArchiveDocument(archiveDocument);
}

ImageIoJob ArchiveDocumentSessionStore::loadArchiveImages(QObject *receiver,
    ArchiveDocumentLocation archiveDocument, ImageCandidatesCallback callback,
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
