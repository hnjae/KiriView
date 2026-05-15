// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIME_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIME_H

#include "imageasyncdependencies.h"
#include "imagedocumenteffects.h"
#include "imagedocumentstate.h"
#include "imagedocumenttypes.h"

#include <QString>
#include <functional>
#include <memory>

class QObject;

namespace KiriView {
class ArchiveDocumentSessionStore;
class ImageDocumentDeletionController;
class ImageDocumentEffectExecutor;
class ImageDocumentLoadController;
class ImageDocumentNavigationController;
class ImageDocumentNavigationCoordinator;
class ImageDocumentPredecodeController;
class ImageOpenController;
class ImagePresentationController;
class ImageSpreadPresentationController;

struct ImageDocumentRuntime final {
    using RenderContextProvider = std::function<ImageDocumentRenderContext()>;
    using ChangeCallback = std::function<void(ImageDocumentChange)>;
    using FileDeletionFailedCallback = std::function<void(const QString &)>;

    ImageDocumentRuntime(QObject *documentObject, RenderContextProvider renderContextProvider,
        ChangeCallback changeCallback, ImageAsyncDependencies dependencies,
        FileDeletionFailedCallback fileDeletionFailedCallback);
    ~ImageDocumentRuntime();

    void dispatchEffect(ImageDocumentEffect effect);
    void notify(ImageDocumentChange change);
    void shutdown();

    ImageDocumentState state;
    ChangeCallback changeCallback;
    std::unique_ptr<ArchiveDocumentSessionStore> archiveSessionStore;
    std::unique_ptr<ImageDocumentDeletionController> documentDeletionController;
    std::unique_ptr<ImagePresentationController> presentationController;
    std::unique_ptr<ImageOpenController> openController;
    std::unique_ptr<ImageDocumentNavigationController> navigationController;
    std::unique_ptr<ImageDocumentPredecodeController> predecodeController;
    std::unique_ptr<ImageSpreadPresentationController> spreadController;
    std::unique_ptr<ImageDocumentLoadController> loadController;
    std::unique_ptr<ImageDocumentEffectExecutor> effectExecutor;
    std::unique_ptr<ImageDocumentNavigationCoordinator> navigationCoordinator;
};
}

#endif
