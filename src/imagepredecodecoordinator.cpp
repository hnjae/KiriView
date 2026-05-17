// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepredecodecoordinator.h"

#include "imageurl.h"

#include <QThread>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::ImageCandidateListContext;

KiriView::PredecodePolicyInput predecodePolicyInput(
    const KiriView::ArchiveDocumentLocation &archiveDocument,
    KiriView::PredecodeMomentumMode momentumMode, bool powerSaverEnabled)
{
    return KiriView::predecodePolicyInputForArchiveDocument(
        archiveDocument, momentumMode, powerSaverEnabled, QThread::idealThreadCount());
}

std::optional<std::size_t> currentCandidateIndex(
    const std::vector<KiriView::ImageNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    const auto currentCandidate = std::find_if(candidates.cbegin(), candidates.cend(),
        [&currentUrl](const KiriView::ImageNavigationCandidate &candidate) {
            return KiriView::sameNormalizedUrl(candidate.url, currentUrl);
        });
    if (currentCandidate == candidates.cend()) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(std::distance(candidates.cbegin(), currentCandidate));
}

std::vector<QUrl> predecodeWindowImageUrls(
    const std::vector<KiriView::ImageNavigationCandidate> &candidates,
    const std::vector<std::size_t> &indices)
{
    std::vector<QUrl> urls;
    urls.reserve(indices.size());
    for (std::size_t index : indices) {
        if (index < candidates.size()) {
            urls.push_back(candidates.at(index).url);
        }
    }
    return urls;
}

}

namespace KiriView {
ImagePredecodeCoordinator::ImagePredecodeCoordinator(QObject *parent)
    : ImagePredecodeCoordinator(
          parent, ImageNavigationCandidateProvider {}, ImageDecodeDependencies {})
{
}

ImagePredecodeCoordinator::ImagePredecodeCoordinator(QObject *parent,
    ImageNavigationCandidateProvider candidateProvider, ImageDecodeDependencies decodeDependencies)
    : QObject(parent)
    , m_candidateRepository(std::move(candidateProvider))
    , m_loadController(this, std::move(decodeDependencies))
{
    m_monotonicClock.start();
    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(predecodeDebounceMsec());
    m_neutralTimer.setSingleShot(true);
    m_neutralTimer.setInterval(predecodeNeutralRefreshMsec());

    QObject::connect(
        &m_debounceTimer, &QTimer::timeout, this, [this]() { startDebouncedPredecode(); });
    QObject::connect(
        &m_neutralTimer, &QTimer::timeout, this, [this]() { scheduleSettledNeutralPredecode(); });
}

void ImagePredecodeCoordinator::schedule(Context context)
{
    cancelBackgroundWork();
    if (!context.primaryImage.hasLocation() || !context.primaryImage.hasStaticImage()) {
        return;
    }

    const quint64 generation = m_generation.next();
    updateNavigationMomentum(context.pageIndex, currentMonotonicMsec());
    cacheDisplayedImages(context);
    m_displayedContext = context;
    if (m_powerSaverEnabled) {
        m_loadController.clearWindowUrls();
        return;
    }

    m_pendingContext = std::move(context);
    m_pendingGeneration = generation;
    m_debounceTimer.start();
    m_neutralTimer.start();
}

void ImagePredecodeCoordinator::setPowerSaverEnabled(bool enabled)
{
    if (m_powerSaverEnabled == enabled) {
        return;
    }

    m_powerSaverEnabled = enabled;
    if (enabled) {
        cancelBackgroundWork();
        m_loadController.clearWindowUrls();
        return;
    }

    if (m_displayedContext.has_value()) {
        schedule(*m_displayedContext);
    }
}

bool ImagePredecodeCoordinator::powerSaverEnabled() const { return m_powerSaverEnabled; }

void ImagePredecodeCoordinator::cacheDisplayedImages(const Context &context)
{
    m_loadController.cacheDisplayedImages(displayedImages(context));
}

std::vector<DisplayedPredecodeImage> ImagePredecodeCoordinator::displayedImages(
    const Context &context) const
{
    std::vector<DisplayedPredecodeImage> images;
    images.push_back(context.primaryImage);
    if (context.secondaryImage.has_value()) {
        images.push_back(*context.secondaryImage);
    }

    return images;
}

void ImagePredecodeCoordinator::startDebouncedPredecode()
{
    if (!m_pendingContext.has_value() || !m_generation.accepts(m_pendingGeneration)) {
        return;
    }

    scheduleAdjacentImagePredecode(*m_pendingContext, m_pendingGeneration);
}

void ImagePredecodeCoordinator::scheduleSettledNeutralPredecode()
{
    if (!m_pendingContext.has_value() || m_momentumState.mode == PredecodeMomentumMode::Neutral) {
        return;
    }

    const Context context = *m_pendingContext;
    m_debounceTimer.stop();
    m_listerJob.cancel();
    m_loadController.cancelBackgroundWork();
    m_momentumState.mode = PredecodeMomentumMode::Neutral;
    m_pendingGeneration = m_generation.next();
    m_pendingContext = context;
    scheduleAdjacentImagePredecode(context, m_pendingGeneration);
}

void ImagePredecodeCoordinator::scheduleAdjacentImagePredecode(
    const Context &context, quint64 generation)
{
    const PredecodePolicyInput policyInput = predecodePolicyInput(
        context.primaryImage.location.archiveDocument(), m_momentumState.mode, m_powerSaverEnabled);
    const PredecodeSchedulePlan initialPlan = predecodeSchedulePlan(0, std::nullopt, policyInput);
    const std::size_t parallelLimit = initialPlan.parallelLimit;
    if (parallelLimit == 0) {
        startPredecodeImageLoads({}, context.primaryImage.location.archiveDocument(), context,
            generation, parallelLimit);
        return;
    }

    const std::optional<ImageCandidateListContext> candidateContext
        = imageCandidateListContextForDisplayedImage(context.primaryImage.location);
    if (!candidateContext.has_value()) {
        startPredecodeImageLoads({}, context.primaryImage.location.archiveDocument(), context,
            generation, parallelLimit);
        return;
    }

    const QUrl currentUrl = candidateContext->currentUrl();
    const ArchiveDocumentLocation archiveDocument = candidateContext->archiveDocument();
    m_listerJob = m_candidateRepository.loadImages(
        this, *candidateContext,
        [this, context, generation, currentUrl, archiveDocument, policyInput](
            std::vector<ImageNavigationCandidate> candidates) {
            const PredecodeSchedulePlan plan = predecodeSchedulePlan(
                candidates.size(), currentCandidateIndex(candidates, currentUrl), policyInput);
            startPredecodeImageLoads(predecodeWindowImageUrls(candidates, plan.targetIndices),
                archiveDocument, context, generation, plan.parallelLimit);
        },
        [this, context, generation, archiveDocument, parallelLimit](const QString &) {
            startPredecodeImageLoads({}, archiveDocument, context, generation, parallelLimit);
        });
}

void ImagePredecodeCoordinator::startPredecodeImageLoads(const std::vector<QUrl> &urls,
    const ArchiveDocumentLocation &archiveDocument, const Context &context, quint64 generation,
    std::size_t parallelLimit)
{
    if (!m_generation.accepts(generation)) {
        return;
    }

    m_loadController.startWindowLoads(PredecodeLoadWindow {
        context.primaryImage.location.imageUrl(),
        archiveDocument,
        urls,
        displayedImages(context),
        context.firstDisplayContext,
        generation,
        parallelLimit,
    });
}

void ImagePredecodeCoordinator::cancelBackgroundWork()
{
    m_generation.invalidate();
    m_debounceTimer.stop();
    m_neutralTimer.stop();
    m_listerJob.cancel();
    m_loadController.cancelBackgroundWork();
    m_pendingContext.reset();
    m_pendingGeneration = 0;
}

void ImagePredecodeCoordinator::resetNavigationMomentum() { m_momentumState = {}; }

void ImagePredecodeCoordinator::updateNavigationMomentum(int pageIndex, qint64 monotonicMsec)
{
    m_momentumState = predecodeUpdatedMomentumState(m_momentumState, pageIndex, monotonicMsec);
}

qint64 ImagePredecodeCoordinator::currentMonotonicMsec() const
{
    return m_monotonicClock.isValid() ? m_monotonicClock.elapsed() : 0;
}

void ImagePredecodeCoordinator::cancel()
{
    cancelBackgroundWork();
    m_displayedContext.reset();
    resetNavigationMomentum();
}

void ImagePredecodeCoordinator::clear()
{
    cancel();
    m_loadController.clear();
}

std::optional<PredecodedImage> ImagePredecodeCoordinator::tryTake(const QUrl &url) const
{
    return m_loadController.tryTake(url);
}
}
