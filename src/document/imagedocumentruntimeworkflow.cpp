// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntimeworkflow.h"

#include "archive/mediaentrysourcestore.h"
#include "imagedocumentdeletioncontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentsourceloadrequest.h"
#include "imagedocumentsourceloadscope.h"
#include "imagedocumentstate.h"
#include "imageopencontroller.h"
#include "localization/activenavigationboundarytext.h"
#include "navigation/navigationlogging.h"
#include "presentation/imagepagesurfacecontroller.h"
#include "presentation/imagespreadpresentationcontroller.h"

#include <QDebug>
#include <optional>
#include <utility>

namespace {
kiriview::ImageDocumentRuntimeOperations runtimeOperations(
    kiriview::ImageDocumentRuntimeWorkflowPorts ports)
{
    kiriview::ImageDocumentRuntimeOperations operations;
    operations.lifecycle.cancelFileDeletion = [ports]() {
        if (ports.deletionController != nullptr) {
            ports.deletionController->cancel();
        }
    };
    operations.lifecycle.stopPresentationAnimation = [ports]() {
        if (ports.pageSurfaceController != nullptr) {
            ports.pageSurfaceController->stopAnimation();
        }
    };
    operations.lifecycle.shutdownSpread = [ports]() {
        if (ports.spreadController != nullptr) {
            ports.spreadController->shutdown();
        }
    };
    operations.mediaEntrySource.clear = [ports]() {
        if (ports.mediaEntrySourceStore != nullptr) {
            ports.mediaEntrySourceStore->clear();
        }
    };
    operations.predecode.clearPredecode = [ports]() {
        if (ports.predecodeController != nullptr) {
            ports.predecodeController->clear();
        }
    };
    operations.predecode.cancelPredecode = [ports]() {
        if (ports.predecodeController != nullptr) {
            ports.predecodeController->cancel();
        }
    };
    operations.predecode.scheduleAdjacentImagePredecode
        = [ports](const kiriview::ScheduleAdjacentImagePredecodeOperation& operation) {
              if (ports.predecodeController != nullptr && ports.spreadController != nullptr) {
                  std::optional<kiriview::DisplayedPredecodeImage> secondaryImage
                      = ports.spreadController->secondaryDisplayedPredecodeImage();
                  if (operation.target.has_value()) {
                      ports.predecodeController->scheduleImageNavigationTargetPredecode(
                          *operation.target, operation.targetPageIndex, std::move(secondaryImage));
                      return;
                  }

                  ports.predecodeController->scheduleAdjacentImagePredecode(
                      std::move(secondaryImage));
              }
          };
    operations.spread.finishSpreadTransition = [ports]() {
        if (ports.spreadController != nullptr) {
            ports.spreadController->finishTransition();
        }
    };
    operations.spread.resetRightToLeftReading = [ports]() {
        if (ports.spreadController != nullptr) {
            ports.spreadController->resetRightToLeftReading();
        }
    };
    operations.spread.clearSecondaryPage = [ports]() {
        if (ports.spreadController != nullptr) {
            ports.spreadController->clearSecondaryPage();
        }
    };
    operations.spread.beginSameScopeImageNavigationPresentation = [ports]() {
        if (ports.spreadController != nullptr) {
            ports.spreadController->beginSameScopeImageNavigationPresentation();
        }
    };
    operations.spread.notifyRightToLeftReadingChanged = [ports]() {
        if (ports.spreadController != nullptr) {
            ports.spreadController->notifyRightToLeftReadingChanged();
        }
    };
    operations.spread.resetZoom = [ports]() {
        if (ports.spreadController != nullptr) {
            ports.spreadController->resetZoom();
        }
    };
    operations.spread.prepareFailedContainer = [ports](const QUrl&) {
        if (ports.spreadController != nullptr) {
            ports.spreadController->resetZoom();
        }
    };
    operations.navigation.cancelPageNavigationUpdate = [ports]() {
        if (ports.navigationController != nullptr) {
            ports.navigationController->cancelPageNavigationUpdate();
        }
    };
    operations.navigation.cancelNavigation = [ports]() {
        if (ports.navigationController != nullptr) {
            ports.navigationController->cancelNavigation();
        }
    };
    operations.navigation.cancelContainerNavigation = [ports]() {
        if (ports.navigationController != nullptr) {
            ports.navigationController->cancelContainerNavigation();
        }
    };
    operations.navigation.cancelAllNavigation = [ports]() {
        if (ports.navigationController != nullptr) {
            ports.navigationController->cancelAllNavigation();
        }
    };
    operations.navigation.clearPageNavigation = [ports]() {
        if (ports.navigationController != nullptr) {
            ports.navigationController->clearPageNavigation();
        }
    };
    operations.navigation.updatePageNavigation = [ports]() {
        if (ports.navigationController != nullptr) {
            ports.navigationController->updatePageNavigation();
        }
    };
    operations.navigation.loadUrl = [ports](const kiriview::ImageDocumentPageTarget& target) {
        if (ports.loadSource) {
            ports.loadSource(kiriview::ImageDocumentSourceLoadRequest::fromTarget(target));
        }
    };
    operations.navigation.loadContainerImage
        = [ports](const kiriview::ImageDocumentPageTarget& target, const QUrl& containerUrl) {
              if (ports.loadSource) {
                  ports.loadSource(kiriview::ImageDocumentSourceLoadRequest::fromContainerTarget(
                      target, containerUrl));
              }
          };
    operations.navigation.finishEmptyContainerNavigation = [ports](const QUrl& containerUrl) {
        if (ports.openController != nullptr) {
            ports.openController->finishContainerNavigationWithEmptyContainer(containerUrl);
        }
    };
    operations.navigation.finishContainerNavigationLoadWithError = [ports](const QUrl& containerUrl,
                                                                       const QString& errorString) {
        if (ports.openController != nullptr) {
            ports.openController->finishContainerNavigationLoadWithError(containerUrl, errorString);
        }
    };
    operations.navigation.reportContainerNavigationBoundary
        = [ports](kiriview::NavigationDirection direction) {
              if (ports.containerNavigationBoundaryReached) {
                  ports.containerNavigationBoundaryReached(
                      kiriview::containerNavigationBoundaryFeedbackText(direction));
              }
          };
    operations.navigation.reportContainerNavigationListFailure
        = [](const kiriview::ContainerNavigationListFailure& failure) {
              qCDebug(kiriviewNavigationLog)
                  << "container navigation listing failed"
                  << "currentContainerUrl" << failure.currentContainerUrl << "parentUrl"
                  << failure.parentUrl << "direction" << static_cast<int>(failure.direction)
                  << "kind" << static_cast<int>(failure.kind) << "detail"
                  << failure.diagnosticDetail;
          };
    operations.navigation.loadPageNavigationUrl =
        [ports](
            const kiriview::ImageDocumentPageTarget& target, bool preserveTwoPageSpreadTransition) {
            if (ports.loadSource) {
                ports.loadSource(kiriview::ImageDocumentSourceLoadRequest::fromPageNavigationTarget(
                    target, preserveTwoPageSpreadTransition));
            }
        };
    operations.open.cancelOpen = [ports]() {
        if (ports.openController != nullptr) {
            ports.openController->cancel();
        }
    };
    operations.open.clearDisplayedImageLocation = [ports]() {
        if (ports.state != nullptr) {
            ports.state->clearDisplayedImageLocation();
        }
    };
    operations.open.clearPresentationImage = [ports]() {
        if (ports.pageSurfaceController != nullptr) {
            ports.pageSurfaceController->clearImage();
        }
        if (ports.spreadController != nullptr) {
            ports.spreadController->clearPrimaryPageSlot();
        }
    };
    operations.sourceLoad.clearLoadingContainerNavigationUrl = [ports]() {
        if (ports.state != nullptr) {
            ports.state->clearLoadingContainerNavigationUrl();
        }
    };
    operations.sourceLoad.setLoadingContainerNavigationUrl = [ports](const QUrl& url) {
        if (ports.state != nullptr) {
            ports.state->setLoadingContainerNavigationUrl(url);
        }
    };
    operations.sourceLoad.setContainerNavigationUrl = [ports](const QUrl& url) {
        if (ports.state != nullptr) {
            ports.state->setContainerNavigationUrl(url);
        }
    };
    operations.sourceLoad.prepareSourceLoad
        = [ports](const kiriview::ImageDocumentSourceLoadRequest& request) {
              if (ports.mediaEntrySourceStore != nullptr && ports.state != nullptr) {
                  ports.mediaEntrySourceStore->prepareForOpenedCollectionScope(
                      kiriview::openedCollectionScopeForImageDocumentSourceLoad(
                          request, ports.state->displayedOpenedCollectionScope()));
              }
          };
    operations.open.setSourceUrl = [ports](const kiriview::ImageDocumentPageTarget& target) {
        if (ports.state != nullptr) {
            ports.state->setSourceKind(target.kind);
            ports.state->setSourceUrl(target.url);
        }
    };
    operations.sourceLoad.beginOpen = [ports]() {
        if (ports.openController != nullptr) {
            ports.openController->open();
        }
    };
    operations.open.setErrorString = [ports](const QString& errorString) {
        if (ports.state != nullptr) {
            ports.state->setErrorString(errorString);
        }
    };
    operations.open.finishEmptySourceLoad = [ports]() {
        if (ports.openController != nullptr) {
            ports.openController->finishEmptySourceLoad();
        }
    };
    return operations;
}
}

namespace kiriview {
ImageDocumentRuntimeWorkflow::ImageDocumentRuntimeWorkflow(
    ImageDocumentRuntimeOperations operations)
    : m_executor(std::move(operations))
{
}

ImageDocumentRuntimeWorkflow::ImageDocumentRuntimeWorkflow(ImageDocumentRuntimeWorkflowPorts ports)
    : ImageDocumentRuntimeWorkflow(runtimeOperations(std::move(ports)))
{
}

void ImageDocumentRuntimeWorkflow::dispatchPlan(const ImageDocumentRuntimePlan& plan)
{
    m_executor.dispatchPlan(plan);
}

void ImageDocumentRuntimeWorkflow::shutdownRuntime() { m_executor.shutdownRuntime(); }
}
