// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntimedependencies.h"

#include "archive/archivedocumentsessionstore.h"
#include "predecode/predecodecachebudget.h"

#include <utility>

namespace {
bool shouldUseArchiveSessionStore(
    const KiriView::ImageDocumentRuntimeDependencyOverrides &overrides)
{
    return overrides.archiveDocumentSessions
        || (!overrides.candidateProvider.archiveImages && !overrides.imageDecode.dataLoader);
}

}

namespace KiriView {
ImageDocumentRuntimeDependencies::~ImageDocumentRuntimeDependencies() = default;

ImageDocumentRuntimeDependencies resolveImageDocumentRuntimeDependencies(
    ImageDocumentRuntimeDependencyOverrides overrides, QObject *parent)
{
    const bool useArchiveSessionStore = shouldUseArchiveSessionStore(overrides);
    ArchiveDocumentSessionFactory archiveDocumentSessions
        = std::move(overrides.archiveDocumentSessions);
    overrides.archiveDocumentSessions = {};
    overrides.candidateProvider
        = imageNavigationCandidateProviderWithDefaults(std::move(overrides.candidateProvider));
    overrides.imageDecode = imageDecodeDependenciesWithDefaults(std::move(overrides.imageDecode));
    overrides.fileOperations
        = fileOperationProviderWithDefault(std::move(overrides.fileOperations));
    overrides.powerSaver = powerSaverProviderWithDefault(std::move(overrides.powerSaver));
    overrides.predecodeCacheByteBudget
        = KiriView::resolvedPredecodeCacheByteBudget(overrides.predecodeCacheByteBudget);

    std::unique_ptr<ArchiveDocumentSessionStore> archiveSessionStore;
    if (useArchiveSessionStore) {
        archiveSessionStore = std::make_unique<ArchiveDocumentSessionStore>(
            std::move(archiveDocumentSessions), parent);
        overrides.candidateProvider
            = archiveSessionStore->wrapCandidateProvider(std::move(overrides.candidateProvider));
        overrides.imageDecode
            = archiveSessionStore->wrapDecodeDependencies(std::move(overrides.imageDecode));
    }

    return ImageDocumentRuntimeDependencies {
        std::move(overrides.candidateProvider),
        std::move(overrides.imageDecode),
        std::move(overrides.fileOperations),
        std::move(overrides.powerSaver),
        overrides.predecodeCacheByteBudget,
        std::move(archiveSessionStore),
        std::move(overrides.externalPredecodedImageFinder),
    };
}
}
