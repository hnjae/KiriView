// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntimedependencies.h"

#include "archive/mediaentrysourcestore.h"
#include "predecode/predecodecachebudget.h"

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

ImageDocumentRuntimeDependencies resolveImageDocumentRuntimeDependencies(
    ImageDocumentRuntimeDependencyOverrides overrides, QObject *parent)
{
    const bool useMediaEntrySourceStore = shouldUseMediaEntrySourceStore(overrides);
    MediaEntrySourceFactory mediaEntrySourceFactory = std::move(overrides.mediaEntrySourceFactory);
    overrides.mediaEntrySourceFactory = {};
    overrides.candidateProvider = imageDocumentPageNavigationCandidateProviderWithDefaults(
        std::move(overrides.candidateProvider));
    overrides.imageDecode = imageDecodeDependenciesWithDefaults(std::move(overrides.imageDecode));
    overrides.fileOperations
        = fileOperationProviderWithDefault(std::move(overrides.fileOperations));
    overrides.powerSaver = powerSaverProviderWithDefault(std::move(overrides.powerSaver));
    overrides.predecodeCacheByteBudget
        = KiriView::resolvedPredecodeCacheByteBudget(overrides.predecodeCacheByteBudget);

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
        std::move(overrides.fileOperations),
        std::move(overrides.powerSaver),
        overrides.predecodeCacheByteBudget,
        std::move(mediaEntrySourceStore),
        std::move(overrides.externalPredecodedImageFinder),
    };
}
}
