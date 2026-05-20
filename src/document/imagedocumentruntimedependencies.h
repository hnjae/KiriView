// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIMEDEPENDENCIES_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIMEDEPENDENCIES_H

#include "archive/archivebackend.h"
#include "decoding/imagedecodedependencies.h"
#include "filedeletion.h"
#include "navigation/imagecandidateprovider.h"
#include "system/powersaverprovider.h"

#include <memory>

class QObject;

namespace KiriView {
class ArchiveDocumentSessionStore;

struct ImageDocumentRuntimeDependencyOverrides {
    ImageNavigationCandidateProvider candidateProvider;
    ImageDecodeDependencies imageDecode;
    FileOperationProvider fileOperations;
    ArchiveDocumentSessionFactory archiveDocumentSessions;
    PowerSaverProvider powerSaver;
};

struct ImageDocumentRuntimeDependencies {
    ImageNavigationCandidateProvider candidateProvider;
    ImageDecodeDependencies imageDecode;
    FileOperationProvider fileOperations;
    PowerSaverProvider powerSaver;
    std::unique_ptr<ArchiveDocumentSessionStore> archiveSessionStore;

    ~ImageDocumentRuntimeDependencies();
};

ImageDocumentRuntimeDependencies resolveImageDocumentRuntimeDependencies(
    ImageDocumentRuntimeDependencyOverrides overrides, QObject *parent);
}

#endif
