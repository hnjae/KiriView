// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpagenavigationcontroller.h"

#include "async/imagecallback.h"
#include "document/imageremovalfallback.h"
#include "imagedocumentpagenavigationpolicy.h"

#include <QString>
#include <optional>
#include <utility>

namespace KiriView {
ImageDocumentPageNavigationController::ImageDocumentPageNavigationController(QObject *parent,
    const ImageDocumentPageCandidateRepository &candidateRepository, Callbacks callbacks)
    : QObject(parent)
    , m_candidateRepository(candidateRepository)
    , m_callbacks(std::move(callbacks))
{
}

int ImageDocumentPageNavigationController::currentPageNumber() const
{
    return m_model.currentPageNumber();
}

int ImageDocumentPageNavigationController::pageCount() const { return m_model.pageCount(); }

ImageDocumentPageNavigationSnapshot ImageDocumentPageNavigationController::snapshot() const
{
    return m_model.snapshot();
}

std::optional<QUrl> ImageDocumentPageNavigationController::urlAtPage(int pageNumber) const
{
    return m_model.urlAtPage(pageNumber);
}

std::optional<ImageDocumentPageTarget> ImageDocumentPageNavigationController::targetAtPage(
    int pageNumber) const
{
    return m_model.targetAtPage(pageNumber);
}

std::optional<ImageDocumentPageTarget> ImageDocumentPageNavigationController::selectPage(
    int pageNumber)
{
    const std::optional<ImageDocumentPageTarget> target = m_model.selectPage(pageNumber);
    if (target.has_value()) {
        notifyChanged();
    }
    return target;
}

void ImageDocumentPageNavigationController::openAdjacentPage(
    std::optional<ImageDocumentPageCandidateListContext> context, NavigationDirection direction)
{
    cancelNavigation();

    if (m_model.hasKnownSelection()) {
        const std::optional<ImageDocumentPageTarget> target = m_model.selectAdjacentPage(direction);
        if (target.has_value()) {
            notifyChanged();
            reportNavigationPlan(ImageDocumentPageNavigationPlan { OpenImageDocumentPageUrlEffect {
                *target,
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
            std::vector<ImageDocumentPageCandidate> candidates) mutable {
            finishNavigation(
                std::move(candidates), direction, currentUrl, std::move(candidateSource));
        },
        [](const QString &) {});
}

void ImageDocumentPageNavigationController::update(
    std::optional<ImageDocumentPageCandidateListContext> context)
{
    m_refreshListerJob.cancel();

    if (!context.has_value()) {
        clear();
        return;
    }

    const ImageDocumentPageNavigationRefreshPlan refreshPlan = m_model.beginRefresh(*context);
    if (refreshPlan.changed) {
        notifyChanged();
    }

    watchChanges(*context);

    m_refreshListerJob = m_candidateRepository.loadImages(
        this, *context,
        [this, refreshId = refreshPlan.refreshId, candidateSource = context->source()](
            std::vector<ImageDocumentPageCandidate> candidates) {
            const ImageDocumentPageNavigationRefreshResult refresh
                = m_model.completePendingRefresh(candidates, refreshId, candidateSource);
            if (refresh.accepted && refresh.changed) {
                notifyChanged();
            }
        },
        [](const QString &) {});
}

void ImageDocumentPageNavigationController::cancelNavigation() { m_navigationListerJob.cancel(); }

void ImageDocumentPageNavigationController::cancelUpdate()
{
    m_refreshListerJob.cancel();
    m_changesJob.cancel();
}

void ImageDocumentPageNavigationController::clear()
{
    m_changesJob.cancel();
    if (m_model.clear()) {
        notifyChanged();
    }
}

void ImageDocumentPageNavigationController::finishNavigation(
    std::vector<ImageDocumentPageCandidate> candidates, NavigationDirection direction,
    const QUrl &currentUrl, ImageDocumentPageCandidateListSource candidateSource)
{
    const std::optional<ImageDocumentPageCandidate> candidate
        = adjacentImageDocumentPageCandidate(candidates, currentUrl, direction);
    if (!candidate.has_value()) {
        return;
    }

    const ImageDocumentPageTarget target { candidate->url, candidate->kind };
    if (m_model.completeRefresh(candidates, target.url, std::move(candidateSource))) {
        notifyChanged();
    }
    reportNavigationPlan(
        ImageDocumentPageNavigationPlan { OpenImageDocumentPageUrlEffect { target } });
}

void ImageDocumentPageNavigationController::watchChanges(
    const ImageDocumentPageCandidateListContext &context)
{
    if (m_model.shouldKeepExistingWatcherFor(context) && m_changesJob.isActive()) {
        return;
    }

    m_changesJob.cancel();
    const ImageDocumentPageCandidateListSource source = context.source();
    m_changesJob = m_candidateRepository.watchCandidateChanges(
        this, context,
        [this, source](std::vector<ImageDocumentPageCandidate> candidates) {
            updateFromChangedCandidates(std::move(candidates), source);
        },
        [](const QString &) {});
}

void ImageDocumentPageNavigationController::updateFromChangedCandidates(
    std::vector<ImageDocumentPageCandidate> candidates, ImageDocumentPageCandidateListSource source)
{
    const ImageDocumentPageNavigationRefreshResult refresh
        = m_model.completeWatchedRefreshFromCurrentContext(candidates, std::move(source));
    if (!refresh.accepted) {
        return;
    }

    if (refresh.changed) {
        notifyChanged();
    }

    if (refresh.currentPageRemoved && refresh.context.has_value()) {
        recoverFromCurrentPageRemoved(std::move(candidates), *refresh.context);
    }
}

void ImageDocumentPageNavigationController::notifyChanged()
{
    invokeIfSet(m_callbacks.pageNavigationChanged);
}

void ImageDocumentPageNavigationController::reportNavigationPlan(
    ImageDocumentPageNavigationPlan plan)
{
    invokeIfSet(m_callbacks.navigationPlan, std::move(plan));
}

void ImageDocumentPageNavigationController::recoverFromCurrentPageRemoved(
    std::vector<ImageDocumentPageCandidate> candidates,
    ImageDocumentPageCandidateListContext context)
{
    if (deletionInProgress()) {
        return;
    }

    const ImageRemovalFallback fallback = imageRemovalFallbackForImageContext(context);
    const std::optional<ImageDocumentPageTarget> fallbackTarget
        = imageRemovalFallbackTarget(std::move(candidates), fallback);
    ImageDocumentPageNavigationPlan plan { ClearCurrentImageDocumentPageNavigationEffect {} };
    if (fallbackTarget.has_value()) {
        plan.push_back(OpenImageDocumentPageUrlEffect { *fallbackTarget });
    }
    reportNavigationPlan(std::move(plan));
}

bool ImageDocumentPageNavigationController::deletionInProgress() const
{
    return m_callbacks.deletionInProgress && m_callbacks.deletionInProgress();
}
}
