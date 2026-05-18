// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopentransitionapplier.h"

#include "imagecontainer.h"
#include "imagedocumentstate.h"

#include <optional>
#include <utility>

namespace {
KiriView::ImageDocumentStatus documentStatus(KiriView::ImageOpenStatusTarget status)
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

    return KiriView::ImageDocumentStatus::Null;
}

std::optional<KiriView::ImageDocumentEffect> imageOpenEffect(
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

class ImageOpenTransitionApplier final
{
public:
    explicit ImageOpenTransitionApplier(KiriView::ImageDocumentState &state)
        : m_state(state)
        , m_batch(m_state.beginChangeBatch())
    {
    }

    void apply(const KiriView::ImageOpenTransition &transition,
        const KiriView::ImageOpenTransitionContext &context)
    {
        applyStateDelta(transition.stateDelta, context);
        for (KiriView::ImageOpenEffect effect : transition.effects) {
            applyEffect(effect, context);
        }
    }

    KiriView::ImageDocumentEffects takeEffects() { return std::move(m_effects); }

private:
    void applyStateDelta(const KiriView::ImageOpenStateDelta &delta,
        const KiriView::ImageOpenTransitionContext &context)
    {
        if (trackedLoadCompletionBeforeVisibleState(delta)) {
            applyTrackedLoadCompletion(delta);
            applyContainerNavigationUrlTarget(delta.containerNavigationUrl, context);
            applySourceUrlTarget(delta.sourceUrl, context);
            applyDisplayedLocationTarget(delta.displayedLocation, context);
            applyErrorStringTarget(delta.errorString, context);
            applyStatusTarget(delta.status);
            return;
        }

        applySourceUrlTarget(delta.sourceUrl, context);
        applyDisplayedLocationTarget(delta.displayedLocation, context);
        applyContainerNavigationUrlTarget(delta.containerNavigationUrl, context);
        applyErrorStringTarget(delta.errorString, context);
        if (delta.clearLoadingContainerNavigationUrl) {
            applyTrackedLoadCompletion(delta);
        } else {
            applyLoadingTarget(delta.loading);
        }
        applyStatusTarget(delta.status);
    }

    bool trackedLoadCompletionBeforeVisibleState(const KiriView::ImageOpenStateDelta &delta) const
    {
        return delta.clearLoadingContainerNavigationUrl
            && delta.displayedLocation == KiriView::ImageOpenDisplayedLocationTarget::Unchanged;
    }

    void applyTrackedLoadCompletion(const KiriView::ImageOpenStateDelta &delta)
    {
        if (delta.clearLoadingContainerNavigationUrl) {
            m_state.clearLoadingContainerNavigationUrl();
        }
        applyLoadingTarget(delta.loading);
    }

    void applySourceUrlTarget(
        KiriView::ImageOpenUrlTarget target, const KiriView::ImageOpenTransitionContext &context)
    {
        if (target != KiriView::ImageOpenUrlTarget::Unchanged) {
            m_state.setSourceUrl(context.urlForTarget(target));
        }
    }

    void applyDisplayedLocationTarget(KiriView::ImageOpenDisplayedLocationTarget target,
        const KiriView::ImageOpenTransitionContext &context)
    {
        switch (target) {
        case KiriView::ImageOpenDisplayedLocationTarget::Session:
            if (const KiriView::DisplayedImageLocation *location = context.sessionLocation()) {
                m_state.setDisplayedImageLocation(*location);
            }
            return;
        case KiriView::ImageOpenDisplayedLocationTarget::Unchanged:
            return;
        }
    }

    void applyContainerNavigationUrlTarget(
        KiriView::ImageOpenUrlTarget target, const KiriView::ImageOpenTransitionContext &context)
    {
        if (target != KiriView::ImageOpenUrlTarget::Unchanged) {
            m_state.setContainerNavigationUrl(context.urlForTarget(target));
        }
    }

    void applyLoadingTarget(KiriView::ImageOpenBoolTarget target)
    {
        switch (target) {
        case KiriView::ImageOpenBoolTarget::False:
            m_state.setLoading(false);
            return;
        case KiriView::ImageOpenBoolTarget::True:
            m_state.setLoading(true);
            return;
        case KiriView::ImageOpenBoolTarget::Unchanged:
            return;
        }
    }

    void applyStatusTarget(KiriView::ImageOpenStatusTarget target)
    {
        if (target != KiriView::ImageOpenStatusTarget::Unchanged) {
            m_state.setStatus(documentStatus(target));
        }
    }

    void applyErrorStringTarget(KiriView::ImageOpenErrorStringTarget target,
        const KiriView::ImageOpenTransitionContext &context)
    {
        switch (target) {
        case KiriView::ImageOpenErrorStringTarget::Clear:
            m_state.setErrorString(QString());
            return;
        case KiriView::ImageOpenErrorStringTarget::Provided:
            m_state.setErrorString(context.providedErrorString());
            return;
        case KiriView::ImageOpenErrorStringTarget::Unchanged:
            return;
        }
    }

    void applyEffect(
        KiriView::ImageOpenEffect effect, const KiriView::ImageOpenTransitionContext &context)
    {
        std::optional<KiriView::ImageDocumentEffect> documentEffect
            = imageOpenEffect(effect, context);
        if (documentEffect.has_value()) {
            m_effects.push_back(std::move(*documentEffect));
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
    context.containerUrl = session.request.containerNavigationUrl();
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

QUrl ImageOpenTransitionContext::urlForTarget(ImageOpenUrlTarget target) const
{
    switch (target) {
    case ImageOpenUrlTarget::Empty:
        return QUrl();
    case ImageOpenUrlTarget::SessionImage:
        return session != nullptr ? session->location.imageUrl() : QUrl();
    case ImageOpenUrlTarget::SessionContainerNavigation:
        return session != nullptr ? session->request.containerNavigationUrl() : QUrl();
    case ImageOpenUrlTarget::DerivedContainerNavigation:
        return session != nullptr ? containerNavigationUrlForLocation(session->location) : QUrl();
    case ImageOpenUrlTarget::Container:
        return containerUrl.value_or(QUrl());
    case ImageOpenUrlTarget::Displayed:
        return displayedUrl.value_or(QUrl());
    case ImageOpenUrlTarget::Unchanged:
        break;
    }

    return QUrl();
}

const DisplayedImageLocation *ImageOpenTransitionContext::sessionLocation() const
{
    return session != nullptr ? &session->location : nullptr;
}

QString ImageOpenTransitionContext::providedErrorString() const
{
    return errorString.value_or(QString());
}

ImageDocumentEffects applyImageOpenTransition(ImageDocumentState &state,
    const ImageOpenTransition &transition, const ImageOpenTransitionContext &context)
{
    ImageOpenTransitionApplier applier(state);
    applier.apply(transition, context);
    return applier.takeEffects();
}
}
