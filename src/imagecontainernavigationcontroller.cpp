// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainernavigationcontroller.h"

#include "imagecallback.h"
#include "imagenavigationmodel.h"
#include "imageurl.h"

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

    m_containerListJob = m_candidateRepository.loadContainers(
        this, parentUrl,
        [this, direction, currentContainerUrl](
            std::vector<ContainerNavigationCandidate> candidates) {
            finishContainerNavigation(std::move(candidates), direction, currentContainerUrl);
        },
        [](const QString &) {});
}

void ImageContainerNavigationController::cancel()
{
    m_containerListJob.cancel();
    m_firstImageJob.cancel();
}

void ImageContainerNavigationController::finishContainerNavigation(
    std::vector<ContainerNavigationCandidate> candidates, NavigationDirection direction,
    const QUrl &currentContainerUrl)
{
    const auto target
        = adjacentContainerNavigationCandidate(candidates, currentContainerUrl, direction);
    if (!target.has_value()) {
        return;
    }

    m_firstImageJob = m_candidateRepository.loadFirstImageInContainer(
        this, *target,
        [this](const QUrl &imageUrl, const QUrl &containerUrl) {
            openImageFromContainerNavigation(imageUrl, containerUrl);
        },
        [this](const QUrl &containerUrl, ImageCandidateRepositoryError error,
            const QString &errorString) {
            finishContainerNavigationLoadWithError(containerUrl, error, errorString);
        });
}

void ImageContainerNavigationController::openImageFromContainerNavigation(
    const QUrl &imageUrl, const QUrl &containerUrl)
{
    invokeIfSet(m_callbacks.openContainerImage, imageUrl, containerUrl);
}

void ImageContainerNavigationController::finishContainerNavigationLoadWithError(
    const QUrl &containerUrl, ContainerNavigationError error, const QString &errorString)
{
    invokeIfSet(m_callbacks.containerNavigationError, containerUrl, error, errorString);
}
}
