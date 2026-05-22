// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIMEDEPENDENCIES_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIMEDEPENDENCIES_H

#include "archive/archivebackend.h"
#include "decoding/imagedecodedependencies.h"
#include "filedeletion.h"
#include "navigation/imagecandidateprovider.h"
#include "predecode/predecodedimage.h"
#include "system/powersaverprovider.h"

#include <QUrl>
#include <functional>
#include <memory>
#include <optional>

class QObject;

namespace KiriView {
class ArchiveDocumentSessionStore;

using ExternalPredecodedImageFinder = std::function<std::optional<PredecodedImage>(const QUrl &)>;

struct ImageDocumentRuntimeDependencyOverrides {
    ImageNavigationCandidateProvider candidateProvider;
    ImageDecodeDependencies imageDecode;
    FileOperationProvider fileOperations;
    ArchiveDocumentSessionFactory archiveDocumentSessions;
    PowerSaverProvider powerSaver;
    ExternalPredecodedImageFinder externalPredecodedImageFinder;
};

struct ImageDocumentRuntimeDependencies {
    ImageNavigationCandidateProvider candidateProvider;
    ImageDecodeDependencies imageDecode;
    FileOperationProvider fileOperations;
    PowerSaverProvider powerSaver;
    std::unique_ptr<ArchiveDocumentSessionStore> archiveSessionStore;
    ExternalPredecodedImageFinder externalPredecodedImageFinder;

    ~ImageDocumentRuntimeDependencies();
};

ImageDocumentRuntimeDependencies resolveImageDocumentRuntimeDependencies(
    ImageDocumentRuntimeDependencyOverrides overrides, QObject *parent);
}

#endif
