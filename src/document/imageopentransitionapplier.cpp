// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopentransitionapplier.h"

#include "imagedocumentstate.h"
#include "navigation/imagecontainer.h"

#include <utility>

namespace {
std::optional<KiriView::ImageDocumentStatus> documentStatus(KiriView::ImageOpenStatusTarget status)
{
    switch (status) {
    case KiriView::ImageOpenStatusTarget::Null:
        return KiriView::ImageDocumentStatus::Null;
    case KiriView::ImageOpenStatusTarget::Loading:
        return KiriView::ImageDocumentStatus::Loading;
    case KiriView::ImageOpenStatusTarget::Ready:
        return KiriView::ImageDocumentStatus::Ready;
    case KiriView::ImageOpenStatusTarget::Error:
        return KiriView::ImageDocumentStatus::Error;
    case KiriView::ImageOpenStatusTarget::Unchanged:
        break;
    }

    return std::nullopt;
}

std::optional<bool> boolTarget(KiriView::ImageOpenBoolTarget target)
{
    switch (target) {
    case KiriView::ImageOpenBoolTarget::False:
        return false;
    case KiriView::ImageOpenBoolTarget::True:
        return true;
    case KiriView::ImageOpenBoolTarget::Unchanged:
        break;
    }

    return std::nullopt;
}

std::optional<QString> errorStringTarget(KiriView::ImageOpenErrorStringTarget target,
    const KiriView::ImageOpenTransitionContext &context)
{
    switch (target) {
    case KiriView::ImageOpenErrorStringTarget::Clear:
        return QString();
    case KiriView::ImageOpenErrorStringTarget::Provided:
        return context.errorString;
    case KiriView::ImageOpenErrorStringTarget::Unchanged:
        break;
    }

    return std::nullopt;
}

std::optional<KiriView::DisplayedImageLocation> displayedLocationTarget(
    KiriView::ImageOpenDisplayedLocationTarget target,
    const KiriView::ImageOpenTransitionContext &context)
{
    switch (target) {
    case KiriView::ImageOpenDisplayedLocationTarget::Session:
        if (context.session != nullptr) {
            return context.session->location();
        }
        return std::nullopt;
    case KiriView::ImageOpenDisplayedLocationTarget::Unchanged:
        return std::nullopt;
    }

    return std::nullopt;
}

std::optional<KiriView::ImageDocumentEffect> imageOpenDocumentEffect(
    KiriView::ImageOpenEffect effect, const KiriView::ImageOpenTransitionContext &context)
{
    switch (effect) {
    case KiriView::ImageOpenEffect::ClearImage:
        return KiriView::ImageDocumentEffect::clearImage();
    case KiriView::ImageOpenEffect::ResetZoom:
        return KiriView::ImageDocumentEffect::resetZoom();
    case KiriView::ImageOpenEffect::UpdatePageNavigation:
        return KiriView::ImageDocumentEffect::updatePageNavigation();
    case KiriView::ImageOpenEffect::ScheduleAdjacentImagePredecode:
        return KiriView::ImageDocumentEffect::scheduleAdjacentImagePredecode();
    case KiriView::ImageOpenEffect::PrepareFailedContainer:
        if (context.containerUrl.has_value()) {
            return KiriView::ImageDocumentEffect::prepareFailedContainer(*context.containerUrl);
        }
        return std::nullopt;
    }

    return std::nullopt;
}

std::optional<QUrl> resolvedUrlForTarget(
    KiriView::ImageOpenUrlTarget target, const KiriView::ImageOpenTransitionContext &context)
{
    switch (target) {
    case KiriView::ImageOpenUrlTarget::Empty:
        return QUrl();
    case KiriView::ImageOpenUrlTarget::SessionImage:
        return context.session != nullptr ? std::optional<QUrl>(context.session->imageUrl())
                                          : std::nullopt;
    case KiriView::ImageOpenUrlTarget::SessionContainerNavigation:
        return context.session != nullptr
            ? std::optional<QUrl>(context.session->containerNavigationUrl())
            : std::nullopt;
    case KiriView::ImageOpenUrlTarget::DerivedContainerNavigation:
        return context.session != nullptr
            ? std::optional<QUrl>(containerNavigationUrlForLocation(context.session->location()))
            : std::nullopt;
    case KiriView::ImageOpenUrlTarget::Container:
        return context.containerUrl;
    case KiriView::ImageOpenUrlTarget::Displayed:
        return context.displayedUrl;
    case KiriView::ImageOpenUrlTarget::Unchanged:
        break;
    }

    return std::nullopt;
}

KiriView::ImageOpenResolvedStateDelta resolvedStateDelta(
    const KiriView::ImageOpenStateDelta &delta, const KiriView::ImageOpenTransitionContext &context)
{
    return KiriView::ImageOpenResolvedStateDelta {
        resolvedUrlForTarget(delta.sourceUrl, context),
        displayedLocationTarget(delta.displayedLocation, context),
        resolvedUrlForTarget(delta.containerNavigationUrl, context),
        boolTarget(delta.loading),
        documentStatus(delta.status),
        errorStringTarget(delta.errorString, context),
        delta.clearLoadingContainerNavigationUrl,
    };
}

KiriView::ImageDocumentEffects resolvedEffects(const KiriView::ImageOpenTransition &transition,
    const KiriView::ImageOpenTransitionContext &context)
{
    KiriView::ImageDocumentEffects effects;
    effects.reserve(transition.effects.size());
    for (KiriView::ImageOpenEffect effect : transition.effects) {
        std::optional<KiriView::ImageDocumentEffect> documentEffect
            = imageOpenDocumentEffect(effect, context);
        if (documentEffect.has_value()) {
            effects.push_back(std::move(*documentEffect));
        }
    }
    return effects;
}

class ImageOpenTransitionApplier final
{
public:
    explicit ImageOpenTransitionApplier(KiriView::ImageDocumentState &state)
        : m_state(state)
        , m_batch(m_state.beginChangeBatch())
    {
    }

    void apply(KiriView::ImageOpenApplicationPlan plan)
    {
        applyStateDelta(plan.stateDelta);
        m_effects = std::move(plan.effects);
    }

    KiriView::ImageDocumentEffects takeEffects() { return std::move(m_effects); }

private:
    void applyStateDelta(const KiriView::ImageOpenResolvedStateDelta &delta)
    {
        if (trackedLoadCompletionBeforeVisibleState(delta)) {
            applyTrackedLoadCompletion(delta);
            applyContainerNavigationUrl(delta.containerNavigationUrl);
            applySourceUrl(delta.sourceUrl);
            applyDisplayedLocation(delta.displayedLocation);
            applyErrorString(delta.errorString);
            applyStatus(delta.status);
            return;
        }

        applySourceUrl(delta.sourceUrl);
        applyDisplayedLocation(delta.displayedLocation);
        applyContainerNavigationUrl(delta.containerNavigationUrl);
        applyErrorString(delta.errorString);
        if (delta.clearLoadingContainerNavigationUrl) {
            applyTrackedLoadCompletion(delta);
        } else {
            applyLoading(delta.loading);
        }
        applyStatus(delta.status);
    }

    bool trackedLoadCompletionBeforeVisibleState(
        const KiriView::ImageOpenResolvedStateDelta &delta) const
    {
        return delta.clearLoadingContainerNavigationUrl && !delta.displayedLocation.has_value();
    }

    void applyTrackedLoadCompletion(const KiriView::ImageOpenResolvedStateDelta &delta)
    {
        if (delta.clearLoadingContainerNavigationUrl) {
            m_state.clearLoadingContainerNavigationUrl();
        }
        applyLoading(delta.loading);
    }

    void applySourceUrl(const std::optional<QUrl> &url)
    {
        if (url.has_value()) {
            m_state.setSourceUrl(*url);
        }
    }

    void applyDisplayedLocation(const std::optional<KiriView::DisplayedImageLocation> &location)
    {
        if (location.has_value()) {
            m_state.setDisplayedImageLocation(*location);
        }
    }

    void applyContainerNavigationUrl(const std::optional<QUrl> &url)
    {
        if (url.has_value()) {
            m_state.setContainerNavigationUrl(*url);
        }
    }

    void applyLoading(const std::optional<bool> &loading)
    {
        if (loading.has_value()) {
            m_state.setLoading(*loading);
        }
    }

    void applyStatus(const std::optional<KiriView::ImageDocumentStatus> &status)
    {
        if (status.has_value()) {
            m_state.setStatus(*status);
        }
    }

    void applyErrorString(const std::optional<QString> &errorString)
    {
        if (errorString.has_value()) {
            m_state.setErrorString(*errorString);
        }
    }

    KiriView::ImageDocumentState &m_state;
    KiriView::ImageDocumentState::ChangeBatch m_batch;
    KiriView::ImageDocumentEffects m_effects;
};
}

namespace KiriView {
ImageOpenTransitionContext ImageOpenTransitionContext::successfulImageLoad(
    const ImageLoadSession &session)
{
    ImageOpenTransitionContext context;
    context.session = &session;
    return context;
}

ImageOpenTransitionContext ImageOpenTransitionContext::sourceLoadError(
    const ImageLoadSession &session, const QUrl &displayedUrl, const QString &errorString)
{
    ImageOpenTransitionContext context;
    context.session = &session;
    context.containerUrl = session.containerNavigationUrl();
    context.displayedUrl = displayedUrl;
    context.errorString = errorString;
    return context;
}

ImageOpenTransitionContext ImageOpenTransitionContext::containerNavigationError(
    const QUrl &containerUrl, const QString &errorString)
{
    ImageOpenTransitionContext context;
    context.containerUrl = containerUrl;
    context.errorString = errorString;
    return context;
}

ImageOpenTransitionContext ImageOpenTransitionContext::animationError(const QString &errorString)
{
    ImageOpenTransitionContext context;
    context.errorString = errorString;
    return context;
}

ImageOpenApplicationPlan imageOpenApplicationPlan(
    const ImageOpenTransition &transition, const ImageOpenTransitionContext &context)
{
    return ImageOpenApplicationPlan {
        resolvedStateDelta(transition.stateDelta, context),
        resolvedEffects(transition, context),
    };
}

ImageDocumentEffects applyImageOpenTransition(ImageDocumentState &state,
    const ImageOpenTransition &transition, const ImageOpenTransitionContext &context)
{
    ImageOpenTransitionApplier applier(state);
    applier.apply(imageOpenApplicationPlan(transition, context));
    return applier.takeEffects();
}
}
