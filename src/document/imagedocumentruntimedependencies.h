// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIMEDEPENDENCIES_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIMEDEPENDENCIES_H

#include "imageasyncdependencies.h"

#include <memory>

class QObject;

namespace KiriView {
class ArchiveDocumentSessionStore;

struct ImageDocumentRuntimeDependencies {
    ImageNavigationCandidateProvider candidateProvider;
    ImageDecodeDependencies imageDecode;
    FileOperationProvider fileOperations;
    PowerSaverProvider powerSaver;
    std::unique_ptr<ArchiveDocumentSessionStore> archiveSessionStore;

    ~ImageDocumentRuntimeDependencies();
};

ImageDocumentRuntimeDependencies resolveImageDocumentRuntimeDependencies(
    ImageAsyncDependencies dependencies, QObject *parent);
}

#endif
