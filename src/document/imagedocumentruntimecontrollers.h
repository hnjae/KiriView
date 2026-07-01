// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIMECONTROLLERS_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIMECONTROLLERS_H

#include "archive/mediaentrysourcebackend.h"
#include "imagedocumentruntimedependencies.h"
#include "imagedocumentruntimeplan.h"
#include "imagedocumenttypes.h"
#include "rendering/imagerendercontext.h"

#include <QString>
#include <functional>
#include <memory>

class QObject;

namespace kiriview {
class MediaEntrySourceStore;
class ImageDocumentAdjacentPredecodeSchedulerPort;
class ImageDocumentAnimationLoadErrorPort;
class ImageDocumentCurrentPageNumberPort;
class ImageDocumentDeletionController;
class ImageDocumentDeletionProgressPort;
class ImageDocumentNavigationSnapshotPort;
class ImageDocumentPageCandidateSnapshotPort;
class ImageDocumentPredecodedImageLookup;
class ImageDocumentPrimaryPageSlotPort;
class ImageDocumentRuntimeWorkflow;
class ImageDocumentNavigationController;
class ImageDocumentPredecodeController;
class ImageDocumentState;
struct ImageDocumentSourceLoadRequest;
class ImageDocumentPageNavigationService;
class ImageOpenController;
class ImagePageSurfaceController;
class ImagePresentationRuntime;
class ImageSpreadPresentationController;

struct ImageDocumentRuntimeControllerCallbacks
{
    std::function<ImageDocumentRenderContext()> renderContext;
    std::function<void(ImageDocumentChange)> notify;
    std::function<void(const ImageDocumentSourceLoadRequest&)> loadSource;
    std::function<void(const QString&)> fileDeletionFailed;
    std::function<void(const QString&)> unsupportedOpenedCollectionVideoEntered;
    std::function<void(const QString&)> containerNavigationBoundaryReached;
};

class ImageDocumentRuntimeControllers final
{
public:
    ImageDocumentRuntimeControllers(QObject* documentObject, ImageDocumentState& state,
        ImageDocumentRuntimeDependencyOverrides dependencies,
        ImageDocumentRuntimeControllerCallbacks callbacks);
    ~ImageDocumentRuntimeControllers();

    ImageDocumentDeletionController& deletionController() const;
    ImagePageSurfaceController& pageSurfaceController() const;
    ImagePresentationRuntime& presentationRuntime() const;
    ImageDocumentNavigationController& navigationController() const;
    ImageSpreadPresentationController& spreadController() const;
    MediaEntrySourceVideoPlaybackDeviceResult loadOpenedCollectionVideoPlaybackDevice(
        const OpenedCollectionScopeLocation& openedCollectionScope, const QUrl& videoUrl) const;

    void dispatchPlan(const ImageDocumentRuntimePlan& plan);
    void shutdownRuntime();

private:
    ImageDocumentRuntimeControllerCallbacks m_callbacks;
    std::unique_ptr<MediaEntrySourceStore> m_mediaEntrySourceStore;
    std::unique_ptr<ImageDocumentAnimationLoadErrorPort> m_animationLoadErrorPort;
    std::unique_ptr<ImageDocumentDeletionController> m_deletionController;
    std::unique_ptr<ImageDocumentDeletionProgressPort> m_deletionProgressPort;
    std::unique_ptr<ImagePageSurfaceController> m_pageSurfaceController;
    std::unique_ptr<ImagePresentationRuntime> m_presentationRuntime;
    std::unique_ptr<ImageDocumentPageNavigationService> m_navigationService;
    std::unique_ptr<ImageDocumentNavigationSnapshotPort> m_navigationSnapshotPort;
    std::unique_ptr<ImageDocumentCurrentPageNumberPort> m_currentPageNumberPort;
    std::unique_ptr<ImageDocumentPageCandidateSnapshotPort> m_pageCandidateSnapshotPort;
    std::unique_ptr<ImageDocumentAdjacentPredecodeSchedulerPort> m_adjacentPredecodeSchedulerPort;
    std::unique_ptr<ImageDocumentPredecodeController> m_predecodeController;
    std::unique_ptr<ImageDocumentPredecodedImageLookup> m_predecodedImageLookup;
    std::unique_ptr<ImageSpreadPresentationController> m_spreadController;
    std::unique_ptr<ImageDocumentPrimaryPageSlotPort> m_primaryPageSlotPort;
    std::unique_ptr<ImageOpenController> m_openController;
    std::unique_ptr<ImageDocumentNavigationController> m_navigationController;
    std::unique_ptr<ImageDocumentRuntimeWorkflow> m_runtimeWorkflow;
};
}

#endif
