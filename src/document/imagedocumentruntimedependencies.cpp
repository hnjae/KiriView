// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntimedependencies.h"

#include "archive/mediaentrysourcestore.h"
#include "rendering/staticimage.h"

#include <utility>

namespace {
bool shouldUseMediaEntrySourceStore(
    const KiriView::ImageDocumentRuntimeDependencyOverrides &overrides)
{
    return overrides.mediaEntrySourceFactory
        || (!overrides.candidateProvider.openedCollectionCandidates
            && !overrides.imageDecode.dataLoader);
}

}

namespace KiriView {
ImageDocumentRuntimeDependencies::~ImageDocumentRuntimeDependencies() = default;

ImageCacheBudgetRequest imageDocumentCacheBudgetRequestWithDefaults(ImageCacheBudgetRequest request)
{
    if (request.staticTileCachePreferredByteBudget <= 0) {
        request.staticTileCachePreferredByteBudget = imageFullDecodeFallbackByteLimit;
    }
    return request;
}

ImageCacheBudgets resolveImageDocumentCacheBudgets(ImageCacheBudgetRequest request)
{
    request = imageDocumentCacheBudgetRequestWithDefaults(request);
    return resolvedImageCacheBudgets(request, systemMemorySnapshot());
}

ImageDocumentRuntimeDependencies resolveImageDocumentRuntimeDependencies(
    ImageDocumentRuntimeDependencyOverrides overrides, QObject *parent)
{
    const bool useMediaEntrySourceStore = shouldUseMediaEntrySourceStore(overrides);
    MediaEntrySourceFactory mediaEntrySourceFactory = std::move(overrides.mediaEntrySourceFactory);
    overrides.mediaEntrySourceFactory = {};
    overrides.candidateProvider = imageDocumentPageNavigationCandidateProviderWithDefaults(
        std::move(overrides.candidateProvider));
    overrides.imageDecode = imageDecodeDependenciesWithDefaults(std::move(overrides.imageDecode));
    overrides.fileDeletionProvider
        = fileDeletionProviderWithDefault(std::move(overrides.fileDeletionProvider));
    overrides.powerSaver = powerSaverProviderWithDefault(std::move(overrides.powerSaver));
    const ImageCacheBudgets cacheBudgets
        = resolveImageDocumentCacheBudgets(overrides.cacheBudgetRequest);

    std::unique_ptr<MediaEntrySourceStore> mediaEntrySourceStore;
    if (useMediaEntrySourceStore) {
        mediaEntrySourceStore
            = std::make_unique<MediaEntrySourceStore>(std::move(mediaEntrySourceFactory), parent);
        overrides.candidateProvider
            = mediaEntrySourceStore->wrapCandidateProvider(std::move(overrides.candidateProvider));
        overrides.imageDecode
            = mediaEntrySourceStore->wrapDecodeDependencies(std::move(overrides.imageDecode));
    }

    return ImageDocumentRuntimeDependencies {
        std::move(overrides.candidateProvider),
        std::move(overrides.imageDecode),
        std::move(overrides.fileDeletionProvider),
        std::move(overrides.powerSaver),
        cacheBudgets,
        std::move(mediaEntrySourceStore),
        std::move(overrides.externalPredecodedImageFinder),
        overrides.ordinaryDirectMediaPredecodeEnabled,
    };
}
}
