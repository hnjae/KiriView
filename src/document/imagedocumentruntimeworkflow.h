// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIMEWORKFLOW_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIMEWORKFLOW_H

#include "imagedocumentruntimeplanexecutor.h"

#include <QString>
#include <functional>

class QUrl;

namespace kiriview {
class MediaEntrySourceStore;
class ImageDocumentDeletionController;
class ImageDocumentNavigationController;
class ImageDocumentPredecodeController;
class ImageDocumentState;
struct ImageDocumentSourceLoadRequest;
class ImageOpenController;
class ImagePageSurfaceController;
class ImageSpreadPresentationController;

struct ImageDocumentRuntimeWorkflowPorts {
    ImageDocumentState *state = nullptr;
    MediaEntrySourceStore *mediaEntrySourceStore = nullptr;
    ImageDocumentDeletionController *deletionController = nullptr;
    ImagePageSurfaceController *pageSurfaceController = nullptr;
    ImageOpenController *openController = nullptr;
    ImageDocumentPredecodeController *predecodeController = nullptr;
    ImageSpreadPresentationController *spreadController = nullptr;
    ImageDocumentNavigationController *navigationController = nullptr;
    std::function<void(const ImageDocumentSourceLoadRequest &)> loadSource;
    std::function<void(const QString &)> containerNavigationBoundaryReached;
};

class ImageDocumentRuntimeWorkflow final
{
public:
    explicit ImageDocumentRuntimeWorkflow(ImageDocumentRuntimeOperations operations);
    explicit ImageDocumentRuntimeWorkflow(ImageDocumentRuntimeWorkflowPorts ports);

    void dispatchPlan(const ImageDocumentRuntimePlan &plan);
    void shutdownRuntime();

private:
    ImageDocumentRuntimePlanExecutor m_executor;
};
}

#endif
