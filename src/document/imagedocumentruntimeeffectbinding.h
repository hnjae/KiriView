// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIMEEFFECTBINDING_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIMEEFFECTBINDING_H

#include "imagedocumenteffectexecutor.h"

#include <functional>

namespace KiriView {
class ArchiveDocumentSessionStore;
class ImageDocumentDeletionController;
class ImageDocumentNavigationController;
class ImageDocumentPredecodeController;
class ImageDocumentState;
struct ImageDocumentSourceLoadRequest;
class ImageOpenController;
class ImagePresentationController;
class ImageSpreadPresentationController;

struct ImageDocumentRuntimeEffectBinding {
    ArchiveDocumentSessionStore *archiveSessionStore = nullptr;
    ImageDocumentState &state;
    ImageDocumentDeletionController &deletionController;
    ImagePresentationController &presentationController;
    ImageOpenController &openController;
    ImageDocumentNavigationController &navigationController;
    ImageDocumentPredecodeController &predecodeController;
    ImageSpreadPresentationController &spreadController;
    std::function<void(const ImageDocumentSourceLoadRequest &)> loadSource;
};

ImageDocumentEffectOperations imageDocumentRuntimeEffectOperations(
    ImageDocumentRuntimeEffectBinding binding);
}

#endif
