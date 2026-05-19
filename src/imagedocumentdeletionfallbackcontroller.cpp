// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentdeletionfallbackcontroller.h"

#include "imagecallback.h"

#include <utility>
#include <variant>
#include <vector>

namespace KiriView {
ImageDocumentDeletionFallbackController::ImageDocumentDeletionFallbackController(QObject *parent,
    ImageNavigationCandidateProvider candidateProvider, EffectCallback effectCallback)
    : m_parent(parent)
    , m_candidateRepository(std::move(candidateProvider))
    , m_effectCallback(std::move(effectCallback))
{
}

ImageDocumentDeletionFallbackController::~ImageDocumentDeletionFallbackController() = default;

void ImageDocumentDeletionFallbackController::open(const ImageRemovalFallbackPlan &fallbackPlan)
{
    cancel();

    const quint64 operationId = startOperation();
    std::visit([this, operationId](const auto &plan) { openFallbackPlan(operationId, plan); },
        fallbackPlan);
}

void ImageDocumentDeletionFallbackController::cancel()
{
    m_operationId = 0;
    m_job.cancel();
}

quint64 ImageDocumentDeletionFallbackController::startOperation()
{
    m_operationId = nextOperationId();
    return m_operationId;
}

bool ImageDocumentDeletionFallbackController::acceptsOperation(quint64 operationId) const
{
    return operationId != 0 && operationId == m_operationId;
}

bool ImageDocumentDeletionFallbackController::finishOperation(quint64 operationId)
{
    if (!acceptsOperation(operationId)) {
        return false;
    }

    m_operationId = 0;
    return true;
}

quint64 ImageDocumentDeletionFallbackController::nextOperationId()
{
    ++m_nextOperationId;
    if (m_nextOperationId == 0) {
        ++m_nextOperationId;
    }
    return m_nextOperationId;
}

void ImageDocumentDeletionFallbackController::openFallbackPlan(
    quint64 operationId, const NoImageRemovalFallback &)
{
    finishOperation(operationId);
}

void ImageDocumentDeletionFallbackController::openFallbackPlan(
    quint64 operationId, const ImageRemovalFallback &fallback)
{
    m_job = m_candidateRepository.loadImages(
        m_parent, fallback.imageContext,
        [this, operationId, fallback](std::vector<ImageNavigationCandidate> candidates) {
            if (!acceptsOperation(operationId)) {
                return;
            }

            const std::optional<QUrl> fallbackUrl
                = imageRemovalFallbackUrl(std::move(candidates), fallback);
            finishOperation(operationId);
            if (fallbackUrl.has_value()) {
                reportDocumentEffect(ImageDocumentEffect::openUrl(*fallbackUrl));
            }
        },
        [this, operationId](const QString &) { finishOperation(operationId); });
}

void ImageDocumentDeletionFallbackController::openFallbackPlan(
    quint64 operationId, const ComicBookRemovalFallback &fallback)
{
    if (!fallback.candidateDirectoryUrl.isValid() || fallback.candidateDirectoryUrl.isEmpty()) {
        finishOperation(operationId);
        return;
    }

    m_job = m_candidateRepository.loadContainers(
        m_parent, fallback.candidateDirectoryUrl,
        [this, operationId, fallback](std::vector<ContainerNavigationCandidate> candidates) {
            if (!acceptsOperation(operationId)) {
                return;
            }

            const ComicBookRemovalFallbackCandidates fallbackCandidates
                = comicBookRemovalFallbackCandidates(std::move(candidates), fallback);
            openComicBookFallbackCandidate(
                operationId, fallbackCandidates.preferred, fallbackCandidates.fallback);
        },
        [this, operationId](const QString &) { finishOperation(operationId); });
}

void ImageDocumentDeletionFallbackController::openComicBookFallbackCandidate(quint64 operationId,
    const std::optional<ContainerNavigationCandidate> &candidate,
    const std::optional<ContainerNavigationCandidate> &fallbackCandidate)
{
    if (!acceptsOperation(operationId)) {
        return;
    }

    if (!candidate.has_value()) {
        if (fallbackCandidate.has_value()) {
            openComicBookFallbackCandidate(operationId, fallbackCandidate, std::nullopt);
            return;
        }
        finishOperation(operationId);
        return;
    }

    m_job = m_candidateRepository.loadFirstImageInContainer(
        m_parent, *candidate,
        [this, operationId](const QUrl &imageUrl, const QUrl &containerUrl) {
            if (!acceptsOperation(operationId)) {
                return;
            }

            finishOperation(operationId);
            reportDocumentEffect(
                ImageDocumentEffect::containerImageSelected(imageUrl, containerUrl));
        },
        [this, operationId, fallbackCandidate](
            const QUrl &, ImageCandidateRepositoryError, const QString &) {
            openComicBookFallbackCandidate(operationId, fallbackCandidate, std::nullopt);
        });
}

void ImageDocumentDeletionFallbackController::reportDocumentEffect(ImageDocumentEffect effect)
{
    invokeIfSet(m_effectCallback, std::move(effect));
}
}
