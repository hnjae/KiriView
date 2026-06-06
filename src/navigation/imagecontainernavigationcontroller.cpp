// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainernavigationcontroller.h"

#include "async/imagecallback.h"
#include "imagedocumentpagecandidaterepository.h"
#include "imagedocumentpagenavigationpolicy.h"
#include "location/imageurl.h"

#include <QString>
#include <utility>

namespace {
QUrl parentUrlForAdjacentContainerNavigation(const QUrl &currentContainerUrl)
{
    if (currentContainerUrl.isEmpty()) {
        return {};
    }

    return KiriView::parentUrlForContainerNavigation(currentContainerUrl);
}
}

namespace KiriView {
ImageContainerNavigationController::ImageContainerNavigationController(QObject *parent,
    const ImageDocumentPageCandidateRepository &candidateRepository, Callbacks callbacks)
    : QObject(parent)
    , m_candidateRepository(candidateRepository)
    , m_callbacks(std::move(callbacks))
{
}

bool ImageContainerNavigationController::canOpenAdjacentContainer(const QUrl &currentContainerUrl)
{
    const QUrl parentUrl = parentUrlForAdjacentContainerNavigation(currentContainerUrl);
    return parentUrl.isValid() && !parentUrl.isEmpty();
}

void ImageContainerNavigationController::openAdjacentContainer(
    const QUrl &currentContainerUrl, NavigationDirection direction)
{
    const QUrl parentUrl = parentUrlForAdjacentContainerNavigation(currentContainerUrl);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        return;
    }
    cancel();

    const quint64 operationId = m_navigationState.startNavigation();
    m_containerListJob = m_candidateRepository.loadContainers(
        this, parentUrl,
        [this, operationId, direction, currentContainerUrl](
            std::vector<ContainerNavigationCandidate> candidates) {
            finishContainerNavigation(
                operationId, std::move(candidates), direction, currentContainerUrl);
        },
        [this, operationId, currentContainerUrl, parentUrl, direction](const QString &errorString) {
            finishContainerNavigationListWithError(
                operationId, currentContainerUrl, parentUrl, direction, errorString);
        });
}

void ImageContainerNavigationController::cancel()
{
    m_navigationState.cancel();
    m_containerListJob.cancel();
    m_firstImageJob.cancel();
}

void ImageContainerNavigationController::finishContainerNavigation(quint64 operationId,
    std::vector<ContainerNavigationCandidate> candidates, NavigationDirection direction,
    const QUrl &currentContainerUrl)
{
    if (!m_navigationState.acceptsNavigation(operationId)) {
        return;
    }

    if (!containerNavigationCandidateIndex(candidates, currentContainerUrl).has_value()) {
        m_navigationState.finishNavigation(operationId);
        return;
    }

    const auto target
        = adjacentContainerNavigationCandidate(candidates, currentContainerUrl, direction);
    if (!target.has_value()) {
        if (m_navigationState.finishNavigation(operationId)) {
            reportNavigationPlan(ImageDocumentPageNavigationPlan {
                ReportContainerNavigationBoundaryEffect { direction },
            });
        }
        return;
    }

    loadFirstImageFromContainerNavigation(operationId, *target);
}

void ImageContainerNavigationController::finishContainerNavigationListWithError(quint64 operationId,
    const QUrl &currentContainerUrl, const QUrl &parentUrl, NavigationDirection direction,
    const QString &errorString)
{
    if (!m_navigationState.finishNavigation(operationId)) {
        return;
    }

    reportNavigationPlan(ImageDocumentPageNavigationPlan {
        ReportContainerNavigationListErrorEffect {
            currentContainerUrl,
            parentUrl,
            direction,
            errorString,
        },
    });
}

void ImageContainerNavigationController::loadFirstImageFromContainerNavigation(
    quint64 operationId, const ContainerNavigationCandidate &container)
{
    const ImageContainerOpenPlan plan = imageContainerOpenPlanForCandidate(container);
    if (!plan.shouldLoadCandidates()) {
        finishContainerNavigationLoadWithError(operationId, container.url, plan.error, QString());
        return;
    }

    const QUrl containerUrl = container.url;
    m_firstImageJob = m_candidateRepository.loadImages(
        this, *plan.source,
        [this, operationId, containerUrl](std::vector<ImageDocumentPageCandidate> candidates) {
            finishContainerNavigationImageLoad(operationId, containerUrl, std::move(candidates));
        },
        [this, operationId, containerUrl](const QString &errorString) {
            finishContainerNavigationLoadWithError(
                operationId, containerUrl, ImageContainerOpenError::Generic, errorString);
        });
}

void ImageContainerNavigationController::finishContainerNavigationImageLoad(quint64 operationId,
    const QUrl &containerUrl, std::vector<ImageDocumentPageCandidate> candidates)
{
    const ImageContainerOpenResult result = imageContainerOpenResultForCandidates(candidates);
    if (result.openedImage()) {
        openImageFromContainerNavigation(operationId, *result.target, containerUrl);
        return;
    }

    finishContainerNavigationLoadWithError(operationId, containerUrl, result.error, QString());
}

void ImageContainerNavigationController::openImageFromContainerNavigation(
    quint64 operationId, const ImageDocumentPageTarget &target, const QUrl &containerUrl)
{
    if (!m_navigationState.finishNavigation(operationId)) {
        return;
    }

    reportNavigationPlan(
        ImageDocumentPageNavigationPlan { OpenContainerImageDocumentPageNavigationEffect {
            target,
            containerUrl,
        } });
}

void ImageContainerNavigationController::finishContainerNavigationLoadWithError(quint64 operationId,
    const QUrl &containerUrl, ContainerNavigationError error, const QString &errorString)
{
    if (!m_navigationState.finishNavigation(operationId)) {
        return;
    }

    reportNavigationPlan(ImageDocumentPageNavigationPlan { ReportContainerNavigationErrorEffect {
        containerUrl,
        error,
        errorString,
    } });
}

void ImageContainerNavigationController::reportNavigationPlan(ImageDocumentPageNavigationPlan plan)
{
    invokeIfSet(m_callbacks.navigationPlan, std::move(plan));
}
}
