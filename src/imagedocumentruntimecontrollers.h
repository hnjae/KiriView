// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIMECONTROLLERS_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIMECONTROLLERS_H

#include "imageasyncdependencies.h"
#include "imagedocumenteffectplan.h"
#include "imagedocumenteffects.h"
#include "imagedocumenttypes.h"

#include <QString>
#include <functional>
#include <memory>

class QObject;

namespace KiriView {
class ArchiveDocumentSessionStore;
class ImageDocumentDeletionController;
class ImageDocumentEffectExecutor;
class ImageDocumentNavigationController;
class ImageDocumentPredecodeController;
class ImageDocumentState;
struct ImageDocumentSourceLoadRequest;
class ImageNavigationService;
class ImageOpenController;
class ImagePresentationController;
class ImageSpreadPresentationController;

struct ImageDocumentRuntimeControllerCallbacks {
    std::function<ImageDocumentRenderContext()> renderContext;
    std::function<void(ImageDocumentChange)> notify;
    std::function<void(const ImageDocumentSourceLoadRequest &)> loadSource;
    std::function<void(const QString &)> fileDeletionFailed;
};

class ImageDocumentRuntimeControllers final
{
public:
    ImageDocumentRuntimeControllers(QObject *documentObject, ImageDocumentState &state,
        ImageAsyncDependencies dependencies, ImageDocumentRuntimeControllerCallbacks callbacks);
    ~ImageDocumentRuntimeControllers();

    ImageDocumentDeletionController &deletionController() const;
    ImagePresentationController &presentationController() const;
    ImageDocumentNavigationController &navigationController() const;
    ImageSpreadPresentationController &spreadController() const;

    void dispatchEffect(ImageDocumentEffect effect);
    void dispatchPlan(const ImageDocumentRuntimePlan &plan);
    void shutdownRuntime();

private:
    ImageDocumentRuntimeControllerCallbacks m_callbacks;
    std::unique_ptr<ArchiveDocumentSessionStore> m_archiveSessionStore;
    std::unique_ptr<ImageDocumentDeletionController> m_deletionController;
    std::unique_ptr<ImagePresentationController> m_presentationController;
    std::unique_ptr<ImageOpenController> m_openController;
    std::unique_ptr<ImageNavigationService> m_navigationService;
    std::unique_ptr<ImageDocumentPredecodeController> m_predecodeController;
    std::unique_ptr<ImageSpreadPresentationController> m_spreadController;
    std::unique_ptr<ImageDocumentNavigationController> m_navigationController;
    std::unique_ptr<ImageDocumentEffectExecutor> m_effectExecutor;
};
}

#endif
