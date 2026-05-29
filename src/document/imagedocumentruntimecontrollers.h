// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIMECONTROLLERS_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIMECONTROLLERS_H

#include "imagedocumentruntimedependencies.h"
#include "imagedocumentruntimeplan.h"
#include "imagedocumenttypes.h"
#include "rendering/imagerendercontext.h"

#include <QString>
#include <functional>
#include <memory>

class QObject;

namespace KiriView {
class MediaEntrySourceStore;
class ImageDocumentDeletionController;
class ImageDocumentRuntimePlanExecutor;
struct ImageDocumentRuntimeOperations;
class ImageDocumentNavigationController;
class ImageDocumentPredecodeController;
class ImageDocumentState;
struct ImageDocumentSourceLoadRequest;
class ImageDocumentPageNavigationService;
class ImageOpenController;
class ImagePresentationController;
class ImageSpreadPresentationController;

struct ImageDocumentRuntimeControllerCallbacks {
    std::function<ImageDocumentRenderContext()> renderContext;
    std::function<void(ImageDocumentChange)> notify;
    std::function<void(const ImageDocumentSourceLoadRequest &)> loadSource;
    std::function<void(const QString &)> fileDeletionFailed;
    std::function<void(const QString &)> unsupportedOpenedCollectionVideoEntered;
    std::function<void(const QString &)> containerNavigationBoundaryReached;
};

class ImageDocumentRuntimeControllers final
{
public:
    ImageDocumentRuntimeControllers(QObject *documentObject, ImageDocumentState &state,
        ImageDocumentRuntimeDependencyOverrides dependencies,
        ImageDocumentRuntimeControllerCallbacks callbacks);
    ~ImageDocumentRuntimeControllers();

    ImageDocumentDeletionController &deletionController() const;
    ImagePresentationController &presentationController() const;
    ImageDocumentNavigationController &navigationController() const;
    ImageSpreadPresentationController &spreadController() const;

    void dispatchPlan(const ImageDocumentRuntimePlan &plan);
    void shutdownRuntime();

private:
    ImageDocumentRuntimeOperations runtimeOperations(ImageDocumentState &state);

    ImageDocumentRuntimeControllerCallbacks m_callbacks;
    std::unique_ptr<MediaEntrySourceStore> m_mediaEntrySourceStore;
    std::unique_ptr<ImageDocumentDeletionController> m_deletionController;
    std::unique_ptr<ImagePresentationController> m_presentationController;
    std::unique_ptr<ImageOpenController> m_openController;
    std::unique_ptr<ImageDocumentPageNavigationService> m_navigationService;
    std::unique_ptr<ImageDocumentPredecodeController> m_predecodeController;
    std::unique_ptr<ImageSpreadPresentationController> m_spreadController;
    std::unique_ptr<ImageDocumentNavigationController> m_navigationController;
    std::unique_ptr<ImageDocumentRuntimePlanExecutor> m_runtimePlanExecutor;
};
}

#endif
