// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageremovalfallbackexecutor.h"

#include "imagecallback.h"

#include <utility>
#include <variant>
#include <vector>

namespace KiriView {
ImageRemovalFallbackExecutor::ImageRemovalFallbackExecutor(QObject *receiver,
    ImageNavigationCandidateProvider candidateProvider, EffectCallback effectCallback)
    : m_receiver(receiver)
    , m_candidateRepository(std::move(candidateProvider))
    , m_effectCallback(std::move(effectCallback))
{
}

void ImageRemovalFallbackExecutor::open(const ImageRemovalFallbackPlan &fallbackPlan)
{
    std::visit([this](const auto &plan) { openPlan(plan); }, fallbackPlan);
}

void ImageRemovalFallbackExecutor::cancel() { m_job.cancel(); }

void ImageRemovalFallbackExecutor::openPlan(const NoImageRemovalFallback &) { }

void ImageRemovalFallbackExecutor::openPlan(const ImageRemovalFallback &fallback)
{
    m_job = m_candidateRepository.loadImages(
        m_receiver, fallback.imageContext,
        [this, fallback](std::vector<ImageNavigationCandidate> candidates) {
            const std::optional<QUrl> fallbackUrl
                = imageRemovalFallbackUrl(std::move(candidates), fallback);
            if (fallbackUrl.has_value()) {
                report(ImageDeletionEffect::openImageFallback(*fallbackUrl));
            }
        },
        [](const QString &) {});
}

void ImageRemovalFallbackExecutor::openPlan(const ComicBookRemovalFallback &fallback)
{
    if (!fallback.candidateDirectoryUrl.isValid() || fallback.candidateDirectoryUrl.isEmpty()) {
        return;
    }

    m_job = m_candidateRepository.loadContainers(
        m_receiver, fallback.candidateDirectoryUrl,
        [this, fallback](std::vector<ContainerNavigationCandidate> candidates) {
            const ComicBookRemovalFallbackCandidates fallbackCandidates
                = comicBookRemovalFallbackCandidates(std::move(candidates), fallback);
            openComicBookCandidate(fallbackCandidates.preferred, fallbackCandidates.fallback);
        },
        [](const QString &) {});
}

void ImageRemovalFallbackExecutor::openComicBookCandidate(
    const std::optional<ContainerNavigationCandidate> &candidate,
    const std::optional<ContainerNavigationCandidate> &fallbackCandidate)
{
    if (!candidate.has_value()) {
        if (fallbackCandidate.has_value()) {
            openComicBookCandidate(fallbackCandidate, std::nullopt);
        }
        return;
    }

    m_job = m_candidateRepository.loadFirstImageInContainer(
        m_receiver, *candidate,
        [this](const QUrl &imageUrl, const QUrl &containerUrl) {
            report(ImageDeletionEffect::openContainerImageFallback(imageUrl, containerUrl));
        },
        [this, fallbackCandidate](const QUrl &, ImageCandidateRepositoryError, const QString &) {
            openComicBookCandidate(fallbackCandidate, std::nullopt);
        });
}

void ImageRemovalFallbackExecutor::report(ImageDeletionEffect effect)
{
    invokeIfSet(m_effectCallback, std::move(effect));
}
}
