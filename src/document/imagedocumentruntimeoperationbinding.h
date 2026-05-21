// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIMEOPERATIONBINDING_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIMEOPERATIONBINDING_H

#include "imagedocumentruntimeplanexecutor.h"

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

struct ImageDocumentRuntimeOperationBinding {
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

ImageDocumentRuntimeOperations imageDocumentRuntimeOperations(
    ImageDocumentRuntimeOperationBinding binding);
}

#endif
