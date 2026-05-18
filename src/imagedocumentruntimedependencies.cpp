// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntimedependencies.h"

#include "archivedocumentsessionstore.h"

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
    dependencies = imageAsyncDependenciesWithDefaults(std::move(dependencies));

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
