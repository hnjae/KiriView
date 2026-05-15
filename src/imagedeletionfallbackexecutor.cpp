// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedeletionfallbackexecutor.h"

#include "imagecallback.h"

#include <utility>
#include <variant>
#include <vector>

namespace KiriView {
ImageDeletionFallbackExecutor::ImageDeletionFallbackExecutor(QObject *receiver,
    ImageNavigationCandidateProvider candidateProvider, EffectCallback effectCallback)
    : m_receiver(receiver)
    , m_candidateRepository(std::move(candidateProvider))
    , m_effectCallback(std::move(effectCallback))
{
}

void ImageDeletionFallbackExecutor::open(const DeletionFallbackPlan &fallbackPlan)
{
    std::visit([this](const auto &plan) { openPlan(plan); }, fallbackPlan);
}

void ImageDeletionFallbackExecutor::cancel() { m_job.cancel(); }

void ImageDeletionFallbackExecutor::openPlan(const NoDeletionFallbackPlan &) { }

void ImageDeletionFallbackExecutor::openPlan(const ImageDeletionFallbackPlan &fallbackPlan)
{
    m_job = m_candidateRepository.loadImages(
        m_receiver, fallbackPlan.imageContext,
        [this, fallbackPlan](std::vector<ImageNavigationCandidate> candidates) {
            const std::optional<QUrl> fallbackUrl
                = imageDeletionFallbackUrl(std::move(candidates), fallbackPlan);
            if (fallbackUrl.has_value()) {
                report(ImageDeletionEffect::openImageFallback(*fallbackUrl));
            }
        },
        [](const QString &) {});
}

void ImageDeletionFallbackExecutor::openPlan(const ComicBookDeletionFallbackPlan &fallbackPlan)
{
    if (!fallbackPlan.candidateDirectoryUrl.isValid()
        || fallbackPlan.candidateDirectoryUrl.isEmpty()) {
        return;
    }

    m_job = m_candidateRepository.loadContainers(
        m_receiver, fallbackPlan.candidateDirectoryUrl,
        [this, fallbackPlan](std::vector<ContainerNavigationCandidate> candidates) {
            const ComicBookDeletionFallbackCandidates fallbackCandidates
                = comicBookDeletionFallbackCandidates(std::move(candidates), fallbackPlan);
            openComicBookCandidate(fallbackCandidates.preferred, fallbackCandidates.fallback);
        },
        [](const QString &) {});
}

void ImageDeletionFallbackExecutor::openComicBookCandidate(
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

void ImageDeletionFallbackExecutor::report(ImageDeletionEffect effect)
{
    invokeIfSet(m_effectCallback, std::move(effect));
}
}
