// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIMEEFFECTBINDING_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIMEEFFECTBINDING_H

#include "imagedocumenteffectexecutor.h"

namespace KiriView {
class ArchiveDocumentSessionStore;
class ImageDocumentDeletionController;
class ImageDocumentLoadController;
class ImageDocumentPredecodeController;
class ImageDocumentState;
class ImageNavigationService;
class ImageOpenController;
class ImagePresentationController;
class ImageSpreadPresentationController;

struct ImageDocumentRuntimeEffectBinding {
    ArchiveDocumentSessionStore *archiveSessionStore = nullptr;
    ImageDocumentState &state;
    ImageDocumentDeletionController &deletionController;
    ImagePresentationController &presentationController;
    ImageOpenController &openController;
    ImageNavigationService &navigationService;
    ImageDocumentPredecodeController &predecodeController;
    ImageSpreadPresentationController &spreadController;
    ImageDocumentLoadController &loadController;
};

ImageDocumentEffectOperations imageDocumentRuntimeEffectOperations(
    ImageDocumentRuntimeEffectBinding binding);
}

#endif
