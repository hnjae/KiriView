// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentdeletionfallbackcontroller.h"

#include "async/imagecallback.h"

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

            const std::optional<QUrl> fallbackUrl
                = imageRemovalFallbackUrl(std::move(candidates), fallback);
            m_operation.finish(operationId);
            if (fallbackUrl.has_value()) {
                reportRuntimePlan(ImageDocumentRuntimePlan { LoadUrlOperation { *fallbackUrl } });
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

    m_job = m_candidateRepository.loadFirstImageInContainer(
        m_parent, *candidate,
        [this, operationId](const QUrl &imageUrl, const QUrl &containerUrl) {
            if (!m_operation.accepts(operationId)) {
                return;
            }

            m_operation.finish(operationId);
            reportRuntimePlan(ImageDocumentRuntimePlan {
                LoadContainerImageOperation { imageUrl, containerUrl } });
        },
        [this, operationId, fallbackCandidate](
            const QUrl &, ImageContainerOpenError, const QString &) {
            openComicBookFallbackCandidate(operationId, fallbackCandidate, std::nullopt);
        });
}

void ImageDocumentDeletionFallbackController::reportRuntimePlan(ImageDocumentRuntimePlan plan)
{
    invokeIfSet(m_runtimePlanCallback, std::move(plan));
}
}
