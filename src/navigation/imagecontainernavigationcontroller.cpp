// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainernavigationcontroller.h"

#include "async/imagecallback.h"
#include "imagecandidaterepository.h"
#include "imagenavigationmodel.h"
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
ImageContainerNavigationController::ImageContainerNavigationController(
    QObject *parent, const ImageCandidateRepository &candidateRepository, Callbacks callbacks)
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
        [this, operationId](
            const QString &) { finishContainerNavigationListWithError(operationId); });
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

    const auto target
        = adjacentContainerNavigationCandidate(candidates, currentContainerUrl, direction);
    if (!target.has_value()) {
        m_navigationState.finishNavigation(operationId);
        return;
    }

    m_firstImageJob = m_candidateRepository.loadFirstImageInContainer(
        this, *target,
        [this, operationId](const QUrl &imageUrl, const QUrl &containerUrl) {
            openImageFromContainerNavigation(operationId, imageUrl, containerUrl);
        },
        [this, operationId](
            const QUrl &containerUrl, ImageContainerOpenError error, const QString &errorString) {
            finishContainerNavigationLoadWithError(operationId, containerUrl, error, errorString);
        });
}

void ImageContainerNavigationController::finishContainerNavigationListWithError(quint64 operationId)
{
    m_navigationState.finishNavigation(operationId);
}

void ImageContainerNavigationController::openImageFromContainerNavigation(
    quint64 operationId, const QUrl &imageUrl, const QUrl &containerUrl)
{
    if (!m_navigationState.finishNavigation(operationId)) {
        return;
    }

    invokeIfSet(m_callbacks.openContainerImage, imageUrl, containerUrl);
}

void ImageContainerNavigationController::finishContainerNavigationLoadWithError(quint64 operationId,
    const QUrl &containerUrl, ContainerNavigationError error, const QString &errorString)
{
    if (!m_navigationState.finishNavigation(operationId)) {
        return;
    }

    invokeIfSet(m_callbacks.containerNavigationError, containerUrl, error, errorString);
}
}
