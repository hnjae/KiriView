// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentdeletionfallbackcontroller.h"

#include "async/imagecallback.h"
#include "navigation/imagecontaineropenplan.h"

#include <utility>
#include <variant>
#include <vector>

namespace KiriView {
ImageDocumentDeletionFallbackController::ImageDocumentDeletionFallbackController(QObject *parent,
    ImageNavigationCandidateProvider candidateProvider, RuntimePlanCallback runtimePlanCallback)
    : m_parent(parent)
    , m_candidateRepository(std::move(candidateProvider))
    , m_runtimePlanCallback(std::move(runtimePlanCallback))
{
}

ImageDocumentDeletionFallbackController::~ImageDocumentDeletionFallbackController() = default;

void ImageDocumentDeletionFallbackController::open(const ImageRemovalFallbackPlan &fallbackPlan)
{
    cancel();

    const quint64 operationId = m_operation.start();
    std::visit([this, operationId](const auto &plan) { openFallbackPlan(operationId, plan); },
        fallbackPlan);
}

void ImageDocumentDeletionFallbackController::cancel()
{
    m_operation.cancel();
    m_job.cancel();
}

void ImageDocumentDeletionFallbackController::openFallbackPlan(
    quint64 operationId, const NoImageRemovalFallback &)
{
    m_operation.finish(operationId);
}

void ImageDocumentDeletionFallbackController::openFallbackPlan(
    quint64 operationId, const ImageRemovalFallback &fallback)
{
    m_job = m_candidateRepository.loadImages(
        m_parent, fallback.imageContext,
        [this, operationId, fallback](std::vector<ImageNavigationCandidate> candidates) {
            if (!m_operation.accepts(operationId)) {
                return;
            }

            const std::optional<ImageNavigationTarget> fallbackTarget
                = imageRemovalFallbackTarget(std::move(candidates), fallback);
            m_operation.finish(operationId);
            if (fallbackTarget.has_value()) {
                reportRuntimePlan(
                    ImageDocumentRuntimePlan { LoadUrlOperation { *fallbackTarget } });
            }
        },
        [this, operationId](const QString &) { m_operation.finish(operationId); });
}

void ImageDocumentDeletionFallbackController::openFallbackPlan(
    quint64 operationId, const ComicBookRemovalFallback &fallback)
{
    if (!fallback.candidateDirectoryUrl.isValid() || fallback.candidateDirectoryUrl.isEmpty()) {
        m_operation.finish(operationId);
        return;
    }

    m_job = m_candidateRepository.loadContainers(
        m_parent, fallback.candidateDirectoryUrl,
        [this, operationId, fallback](std::vector<ContainerNavigationCandidate> candidates) {
            if (!m_operation.accepts(operationId)) {
                return;
            }

            const ComicBookRemovalFallbackCandidates fallbackCandidates
                = comicBookRemovalFallbackCandidates(std::move(candidates), fallback);
            openComicBookFallbackCandidate(
                operationId, fallbackCandidates.preferred, fallbackCandidates.fallback);
        },
        [this, operationId](const QString &) { m_operation.finish(operationId); });
}

void ImageDocumentDeletionFallbackController::openComicBookFallbackCandidate(quint64 operationId,
    const std::optional<ContainerNavigationCandidate> &candidate,
    const std::optional<ContainerNavigationCandidate> &fallbackCandidate)
{
    if (!m_operation.accepts(operationId)) {
        return;
    }

    if (!candidate.has_value()) {
        if (fallbackCandidate.has_value()) {
            openComicBookFallbackCandidate(operationId, fallbackCandidate, std::nullopt);
            return;
        }
        m_operation.finish(operationId);
        return;
    }

    loadComicBookFallbackImage(operationId, *candidate, fallbackCandidate);
}

void ImageDocumentDeletionFallbackController::loadComicBookFallbackImage(quint64 operationId,
    const ContainerNavigationCandidate &candidate,
    const std::optional<ContainerNavigationCandidate> &fallbackCandidate)
{
    const ImageContainerOpenPlan plan = imageContainerOpenPlanForCandidate(candidate);
    if (!plan.shouldLoadCandidates()) {
        failComicBookFallbackImageLoad(operationId, fallbackCandidate);
        return;
    }

    const QUrl containerUrl = candidate.url;
    m_job = m_candidateRepository.loadImages(
        m_parent, *plan.source,
        [this, operationId, containerUrl, fallbackCandidate](
            std::vector<ImageNavigationCandidate> candidates) {
            finishComicBookFallbackImageLoad(
                operationId, containerUrl, fallbackCandidate, std::move(candidates));
        },
        [this, operationId, fallbackCandidate](
            const QString &) { failComicBookFallbackImageLoad(operationId, fallbackCandidate); });
}

void ImageDocumentDeletionFallbackController::finishComicBookFallbackImageLoad(quint64 operationId,
    const QUrl &containerUrl, const std::optional<ContainerNavigationCandidate> &fallbackCandidate,
    std::vector<ImageNavigationCandidate> candidates)
{
    if (!m_operation.accepts(operationId)) {
        return;
    }

    const ImageContainerOpenResult result = imageContainerOpenResultForCandidates(candidates);
    if (!result.openedImage()) {
        failComicBookFallbackImageLoad(operationId, fallbackCandidate);
        return;
    }

    m_operation.finish(operationId);
    reportRuntimePlan(
        ImageDocumentRuntimePlan { LoadContainerImageOperation { *result.target, containerUrl } });
}

void ImageDocumentDeletionFallbackController::failComicBookFallbackImageLoad(
    quint64 operationId, const std::optional<ContainerNavigationCandidate> &fallbackCandidate)
{
    if (!m_operation.accepts(operationId)) {
        return;
    }

    openComicBookFallbackCandidate(operationId, fallbackCandidate, std::nullopt);
}

void ImageDocumentDeletionFallbackController::reportRuntimePlan(ImageDocumentRuntimePlan plan)
{
    invokeIfSet(m_runtimePlanCallback, std::move(plan));
}
}
