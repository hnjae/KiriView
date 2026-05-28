// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIMEDEPENDENCIES_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIMEDEPENDENCIES_H

#include "archive/mediaentrysourcebackend.h"
#include "cache/imagecachepolicy.h"
#include "decoding/imagedecodedependencies.h"
#include "filedeletion.h"
#include "navigation/imagedocumentpagecandidateprovider.h"
#include "predecode/predecodedimage.h"
#include "system/powersaverprovider.h"

#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <memory>
#include <optional>

class QObject;

namespace KiriView {
class MediaEntrySourceStore;

using ExternalPredecodedImageFinder = std::function<std::optional<PredecodedImage>(const QUrl &)>;

struct ImageDocumentRuntimeDependencyOverrides {
    ImageDocumentPageCandidateProvider candidateProvider;
    ImageDecodeDependencies imageDecode;
    FileOperationProvider fileOperations;
    MediaEntrySourceFactory mediaEntrySourceFactory;
    PowerSaverProvider powerSaver;
    ExternalPredecodedImageFinder externalPredecodedImageFinder;
    ImageCacheBudgetRequest cacheBudgetRequest;
    bool ordinaryDirectMediaPredecodeEnabled = true;
};

struct ImageDocumentRuntimeDependencies {
    ImageDocumentPageCandidateProvider candidateProvider;
    ImageDecodeDependencies imageDecode;
    FileOperationProvider fileOperations;
    PowerSaverProvider powerSaver;
    ImageCacheBudgets cacheBudgets;
    std::unique_ptr<MediaEntrySourceStore> mediaEntrySourceStore;
    ExternalPredecodedImageFinder externalPredecodedImageFinder;
    bool ordinaryDirectMediaPredecodeEnabled = true;

    ~ImageDocumentRuntimeDependencies();
};

ImageDocumentRuntimeDependencies resolveImageDocumentRuntimeDependencies(
    ImageDocumentRuntimeDependencyOverrides overrides, QObject *parent);
ImageCacheBudgetRequest imageDocumentCacheBudgetRequestWithDefaults(
    ImageCacheBudgetRequest request);
ImageCacheBudgets resolveImageDocumentCacheBudgets(ImageCacheBudgetRequest request);
}

#endif
