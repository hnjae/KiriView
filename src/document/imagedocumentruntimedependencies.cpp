// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntimedependencies.h"

#include "archive/archivedocumentsessionstore.h"

#include <utility>

namespace {
bool shouldUseArchiveSessionStore(const KiriView::ImageAsyncDependencies &dependencies)
{
    return dependencies.archiveDocumentSessions
        || (!dependencies.candidateProvider.archiveImages && !dependencies.imageDecode.dataLoader);
}
}

namespace KiriView {
ImageDocumentRuntimeDependencies::~ImageDocumentRuntimeDependencies() = default;

ImageDocumentRuntimeDependencies resolveImageDocumentRuntimeDependencies(
    ImageAsyncDependencies dependencies, QObject *parent)
{
    const bool useArchiveSessionStore = shouldUseArchiveSessionStore(dependencies);
    ArchiveDocumentSessionFactory archiveDocumentSessions
        = std::move(dependencies.archiveDocumentSessions);
    dependencies.archiveDocumentSessions = {};
    dependencies.candidateProvider
        = imageNavigationCandidateProviderWithDefaults(std::move(dependencies.candidateProvider));
    dependencies.imageDecode
        = imageDecodeDependenciesWithDefaults(std::move(dependencies.imageDecode));
    dependencies.fileOperations
        = fileOperationProviderWithDefault(std::move(dependencies.fileOperations));
    dependencies.powerSaver = powerSaverProviderWithDefault(std::move(dependencies.powerSaver));

    std::unique_ptr<ArchiveDocumentSessionStore> archiveSessionStore;
    if (useArchiveSessionStore) {
        archiveSessionStore = std::make_unique<ArchiveDocumentSessionStore>(
            std::move(archiveDocumentSessions), parent);
        dependencies = archiveSessionStore->wrapDependencies(std::move(dependencies));
    }

    return ImageDocumentRuntimeDependencies {
        std::move(dependencies.candidateProvider),
        std::move(dependencies.imageDecode),
        std::move(dependencies.fileOperations),
        std::move(dependencies.powerSaver),
        std::move(archiveSessionStore),
    };
}
}
