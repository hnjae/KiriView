// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepagenavigationcontroller.h"

#include "async/imagecallback.h"
#include "document/imageremovalfallback.h"
#include "imagenavigationmodel.h"

#include <QString>
#include <optional>
#include <utility>

namespace KiriView {
ImagePageNavigationController::ImagePageNavigationController(
    QObject *parent, const ImageCandidateRepository &candidateRepository, Callbacks callbacks)
    : QObject(parent)
    , m_candidateRepository(candidateRepository)
    , m_callbacks(std::move(callbacks))
{
}

int ImagePageNavigationController::currentPageNumber() const { return m_model.currentPageNumber(); }

int ImagePageNavigationController::imageCount() const { return m_model.imageCount(); }

ImagePageNavigationSnapshot ImagePageNavigationController::snapshot() const
{
    return m_model.snapshot();
}

std::optional<QUrl> ImagePageNavigationController::urlAtPage(int pageNumber) const
{
    return m_model.urlAtPage(pageNumber);
}

std::optional<QUrl> ImagePageNavigationController::selectPage(int pageNumber)
{
    const std::optional<QUrl> targetUrl = m_model.selectPage(pageNumber);
    if (targetUrl.has_value()) {
        notifyChanged();
    }
    return targetUrl;
}

void ImagePageNavigationController::openAdjacentImage(
    std::optional<ImageCandidateListContext> context, NavigationDirection direction)
{
    cancelNavigation();

    if (m_model.hasKnownSelection()) {
        const std::optional<QUrl> targetUrl = m_model.selectAdjacentPage(direction);
        if (targetUrl.has_value()) {
            notifyChanged();
            reportNavigationPlan(ImageNavigationPlan { OpenImageNavigationUrlEffect {
                *targetUrl,
            } });
        }
        return;
    }

    if (!context.has_value()) {
        return;
    }

    m_navigationListerJob = m_candidateRepository.loadImages(
        this, *context,
        [this, direction, currentUrl = context->currentUrl(), candidateSource = context->source()](
            std::vector<ImageNavigationCandidate> candidates) mutable {
            finishNavigation(
                std::move(candidates), direction, currentUrl, std::move(candidateSource));
        },
        [](const QString &) {});
}

void ImagePageNavigationController::update(std::optional<ImageCandidateListContext> context)
{
    m_refreshListerJob.cancel();

    if (!context.has_value()) {
        clear();
        return;
    }

    const ImagePageNavigationRefreshPlan refreshPlan = m_model.beginRefresh(*context);
    if (refreshPlan.changed) {
        notifyChanged();
    }

    watchChanges(*context);

    m_refreshListerJob = m_candidateRepository.loadImages(
        this, *context,
        [this, refreshId = refreshPlan.refreshId, candidateSource = context->source()](
            std::vector<ImageNavigationCandidate> candidates) {
            const ImagePageNavigationRefreshResult refresh
                = m_model.completePendingRefresh(candidates, refreshId, candidateSource);
            if (refresh.accepted && refresh.changed) {
                notifyChanged();
            }
        },
        [](const QString &) {});
}

void ImagePageNavigationController::cancelNavigation() { m_navigationListerJob.cancel(); }

void ImagePageNavigationController::cancelUpdate()
{
    m_refreshListerJob.cancel();
    m_changesJob.cancel();
}

void ImagePageNavigationController::clear()
{
    m_changesJob.cancel();
    if (m_model.clear()) {
        notifyChanged();
    }
}

void ImagePageNavigationController::finishNavigation(
    std::vector<ImageNavigationCandidate> candidates, NavigationDirection direction,
    const QUrl &currentUrl, ImageCandidateListSource candidateSource)
{
    const std::optional<QUrl> targetUrl
        = adjacentImageNavigationUrl(candidates, currentUrl, direction);
    if (!targetUrl.has_value()) {
        return;
    }

    if (m_model.completeRefresh(candidates, *targetUrl, std::move(candidateSource))) {
        notifyChanged();
    }
    reportNavigationPlan(ImageNavigationPlan { OpenImageNavigationUrlEffect { *targetUrl } });
}

void ImagePageNavigationController::watchChanges(const ImageCandidateListContext &context)
{
    if (m_model.shouldKeepExistingWatcherFor(context) && m_changesJob.isActive()) {
        return;
    }

    m_changesJob.cancel();
    const ImageCandidateListSource source = context.source();
    m_changesJob = m_candidateRepository.watchCandidateChanges(
        this, context,
        [this, source](std::vector<ImageNavigationCandidate> candidates) {
            updateFromChangedCandidates(std::move(candidates), source);
        },
        [](const QString &) {});
}

void ImagePageNavigationController::updateFromChangedCandidates(
    std::vector<ImageNavigationCandidate> candidates, ImageCandidateListSource source)
{
    const ImagePageNavigationRefreshResult refresh
        = m_model.completeWatchedRefreshFromCurrentContext(candidates, std::move(source));
    if (!refresh.accepted) {
        return;
    }

    if (refresh.changed) {
        notifyChanged();
    }

    if (refresh.currentImageRemoved && refresh.context.has_value()) {
        recoverFromCurrentImageRemoved(std::move(candidates), *refresh.context);
    }
}

void ImagePageNavigationController::notifyChanged()
{
    invokeIfSet(m_callbacks.pageNavigationChanged);
}

void ImagePageNavigationController::reportNavigationPlan(ImageNavigationPlan plan)
{
    invokeIfSet(m_callbacks.navigationPlan, std::move(plan));
}

void ImagePageNavigationController::recoverFromCurrentImageRemoved(
    std::vector<ImageNavigationCandidate> candidates, ImageCandidateListContext context)
{
    if (deletionInProgress()) {
        return;
    }

    const ImageRemovalFallback fallback = imageRemovalFallbackForImageContext(context);
    const std::optional<QUrl> fallbackUrl
        = imageRemovalFallbackUrl(std::move(candidates), fallback);
    ImageNavigationPlan plan { ClearCurrentImageNavigationEffect {} };
    if (fallbackUrl.has_value()) {
        plan.push_back(OpenImageNavigationUrlEffect { *fallbackUrl });
    }
    reportNavigationPlan(std::move(plan));
}

bool ImagePageNavigationController::deletionInProgress() const
{
    return m_callbacks.deletionInProgress && m_callbacks.deletionInProgress();
}
}
