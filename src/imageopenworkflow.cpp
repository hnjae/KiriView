// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflow.h"

#include "imagecontainer.h"
#include "imagedocumentstate.h"
#include "kiriview/src/imageopenworkflow.cxx.h"

#include <utility>

namespace {
struct ImageOpenTransitionContext {
    const KiriView::ImageLoadSession *session = nullptr;
    const QUrl *containerUrl = nullptr;
    const QUrl *displayedUrl = nullptr;
    const QString *errorString = nullptr;
};

ImageOpenTransitionContext transitionContextForSuccessfulImageLoad(
    const KiriView::ImageLoadSession &session)
{
    ImageOpenTransitionContext context;
    context.session = &session;
    return context;
}

ImageOpenTransitionContext transitionContextForLoadError(const KiriView::ImageLoadSession &session,
    const QUrl &containerUrl, const QUrl &displayedUrl, const QString &errorString)
{
    ImageOpenTransitionContext context;
    context.session = &session;
    context.containerUrl = &containerUrl;
    context.displayedUrl = &displayedUrl;
    context.errorString = &errorString;
    return context;
}

ImageOpenTransitionContext transitionContextForContainerNavigationError(
    const QUrl &containerUrl, const QString &errorString)
{
    ImageOpenTransitionContext context;
    context.containerUrl = &containerUrl;
    context.errorString = &errorString;
    return context;
}

ImageOpenTransitionContext transitionContextForAnimationError(const QString &errorString)
{
    ImageOpenTransitionContext context;
    context.errorString = &errorString;
    return context;
}

KiriView::RustImageOpenLoadErrorRequest sourceLoadErrorRequest(
    bool containerNavigationUrlEmpty, bool hasImage, bool displayedUrlEmpty)
{
    return KiriView::RustImageOpenLoadErrorRequest {
        KiriView::RustImageOpenLoadErrorKind::SourceLoad,
        containerNavigationUrlEmpty,
        hasImage,
        displayedUrlEmpty,
    };
}

KiriView::RustImageOpenLoadErrorRequest loadErrorRequest(KiriView::RustImageOpenLoadErrorKind kind)
{
    return KiriView::RustImageOpenLoadErrorRequest {
        kind,
        true,
        false,
        true,
    };
}

KiriView::ImageDocumentStatus documentStatus(KiriView::RustImageOpenStatusTarget status)
{
    switch (status) {
    case KiriView::RustImageOpenStatusTarget::Null:
        return KiriView::ImageDocumentStatus::Null;
    case KiriView::RustImageOpenStatusTarget::Loading:
        return KiriView::ImageDocumentStatus::Loading;
    case KiriView::RustImageOpenStatusTarget::Ready:
        return KiriView::ImageDocumentStatus::Ready;
    case KiriView::RustImageOpenStatusTarget::Error:
        return KiriView::ImageDocumentStatus::Error;
    case KiriView::RustImageOpenStatusTarget::Unchanged:
        break;
    }

    return KiriView::ImageDocumentStatus::Null;
}

KiriView::ImageOpenRightToLeftReadingNotification imageOpenRightToLeftReadingNotification(
    KiriView::RustImageOpenRightToLeftReadingNotification notification)
{
    switch (notification) {
    case KiriView::RustImageOpenRightToLeftReadingNotification::BeforeOpen:
        return KiriView::ImageOpenRightToLeftReadingNotification::BeforeOpen;
    case KiriView::RustImageOpenRightToLeftReadingNotification::AfterOpen:
        return KiriView::ImageOpenRightToLeftReadingNotification::AfterOpen;
    case KiriView::RustImageOpenRightToLeftReadingNotification::None:
        break;
    }

    return KiriView::ImageOpenRightToLeftReadingNotification::None;
}

KiriView::ImageOpenSourceLoadPlan imageOpenSourceLoadPlan(
    KiriView::RustImageOpenSourceLoadPlan plan)
{
    return KiriView::ImageOpenSourceLoadPlan { plan.finish_spread_transition,
        plan.reset_right_to_left_reading,
        imageOpenRightToLeftReadingNotification(plan.right_to_left_reading_notification),
        plan.clear_loading_container_navigation_url, plan.update_container_navigation_url,
        plan.cancel_navigation_and_predecode, plan.clear_secondary_page,
        plan.set_loading_container_navigation_url, plan.set_source_url, plan.begin_open };
}

QUrl urlForTarget(
    KiriView::RustImageOpenUrlTarget target, const ImageOpenTransitionContext &context)
{
    switch (target) {
    case KiriView::RustImageOpenUrlTarget::Empty:
        return QUrl();
    case KiriView::RustImageOpenUrlTarget::SessionImage:
        return context.session != nullptr ? context.session->location.imageUrl() : QUrl();
    case KiriView::RustImageOpenUrlTarget::SessionContainerNavigation:
        return context.session != nullptr ? context.session->request.containerNavigationUrl()
                                          : QUrl();
    case KiriView::RustImageOpenUrlTarget::DerivedContainerNavigation:
        return context.session != nullptr
            ? KiriView::containerNavigationUrlForLocation(context.session->location)
            : QUrl();
    case KiriView::RustImageOpenUrlTarget::Container:
        return context.containerUrl != nullptr ? *context.containerUrl : QUrl();
    case KiriView::RustImageOpenUrlTarget::Displayed:
        return context.displayedUrl != nullptr ? *context.displayedUrl : QUrl();
    case KiriView::RustImageOpenUrlTarget::Unchanged:
        break;
    }

    return QUrl();
}

class ImageOpenTransition final
{
public:
    explicit ImageOpenTransition(KiriView::ImageDocumentState &state)
        : m_state(state)
        , m_batch(m_state.beginChangeBatch())
    {
    }

    void applyBeginSourceLoad(const KiriView::RustImageOpenTransition &transition)
    {
        const ImageOpenTransitionContext context;
        applyContainerNavigationUrlTarget(transition.container_navigation_url, context);
        applyLoadingTarget(transition.loading);
        applyEffects(transition.effects, context);
        applyStatusTarget(transition.status);
    }

    void applyFinishEmptySourceLoad(const KiriView::RustImageOpenTransition &transition)
    {
        const ImageOpenTransitionContext context;
        applyEffects(transition.effects, context);
        applyTrackedLoadCompletion(transition);
        applyContainerNavigationUrlTarget(transition.container_navigation_url, context);
        applyStatusTarget(transition.status);
    }

    void applyFinishSuccessfulImageLoad(const KiriView::RustImageOpenTransition &transition,
        const ImageOpenTransitionContext &context)
    {
        applySourceUrlTarget(transition.source_url, context);
        applyDisplayedLocationTarget(transition.displayed_location, context);
        applyContainerNavigationUrlTarget(transition.container_navigation_url, context);
        applyErrorStringTarget(transition.error_string, context);
        applyTrackedLoadCompletion(transition);
        applyStatusTarget(transition.status);
        applyEffects(transition.effects, context);
    }

    void applyFinishLoadWithError(const KiriView::RustImageOpenTransition &transition,
        const ImageOpenTransitionContext &context)
    {
        applyEffects(transition.effects, context);
        applyTrackedLoadCompletion(transition);
        applyContainerNavigationUrlTarget(transition.container_navigation_url, context);
        applySourceUrlTarget(transition.source_url, context);
        applyErrorStringTarget(transition.error_string, context);
        applyStatusTarget(transition.status);
    }

    KiriView::ImageDocumentEffects takeEffects() { return std::move(m_effects); }

private:
    void add(KiriView::ImageDocumentEffect effect) { m_effects.push_back(std::move(effect)); }

    void applyTrackedLoadCompletion(const KiriView::RustImageOpenTransition &transition)
    {
        if (transition.clear_loading_container_navigation_url) {
            m_state.clearLoadingContainerNavigationUrl();
        }
        applyLoadingTarget(transition.loading);
    }

    void applySourceUrlTarget(
        KiriView::RustImageOpenUrlTarget target, const ImageOpenTransitionContext &context)
    {
        if (target != KiriView::RustImageOpenUrlTarget::Unchanged) {
            m_state.setSourceUrl(urlForTarget(target, context));
        }
    }

    void applyDisplayedLocationTarget(KiriView::RustImageOpenDisplayedLocationTarget target,
        const ImageOpenTransitionContext &context)
    {
        if (target == KiriView::RustImageOpenDisplayedLocationTarget::SessionLocation
            && context.session != nullptr) {
            m_state.setDisplayedImageLocation(context.session->location);
        }
    }

    void applyContainerNavigationUrlTarget(
        KiriView::RustImageOpenUrlTarget target, const ImageOpenTransitionContext &context)
    {
        if (target != KiriView::RustImageOpenUrlTarget::Unchanged) {
            m_state.setContainerNavigationUrl(urlForTarget(target, context));
        }
    }

    void applyLoadingTarget(KiriView::RustImageOpenBoolTarget target)
    {
        switch (target) {
        case KiriView::RustImageOpenBoolTarget::False:
            m_state.setLoading(false);
            return;
        case KiriView::RustImageOpenBoolTarget::True:
            m_state.setLoading(true);
            return;
        case KiriView::RustImageOpenBoolTarget::Unchanged:
            return;
        }
    }

    void applyStatusTarget(KiriView::RustImageOpenStatusTarget target)
    {
        if (target != KiriView::RustImageOpenStatusTarget::Unchanged) {
            m_state.setStatus(documentStatus(target));
        }
    }

    void applyErrorStringTarget(
        KiriView::RustImageOpenErrorStringTarget target, const ImageOpenTransitionContext &context)
    {
        switch (target) {
        case KiriView::RustImageOpenErrorStringTarget::Clear:
            m_state.setErrorString(QString());
            return;
        case KiriView::RustImageOpenErrorStringTarget::Provided:
            m_state.setErrorString(
                context.errorString != nullptr ? *context.errorString : QString());
            return;
        case KiriView::RustImageOpenErrorStringTarget::Unchanged:
            return;
        }
    }

    void applyEffects(
        const KiriView::RustImageOpenEffects &effects, const ImageOpenTransitionContext &context)
    {
        if (effects.clear_image) {
            add(KiriView::ImageDocumentEffect::clearImage());
        }
        if (effects.reset_zoom) {
            add(KiriView::ImageDocumentEffect::resetZoom());
        }
        if (effects.update_page_navigation) {
            add(KiriView::ImageDocumentEffect::updatePageNavigation());
        }
        if (effects.schedule_adjacent_image_predecode) {
            add(KiriView::ImageDocumentEffect::scheduleAdjacentImagePredecode());
        }
        if (effects.prepare_failed_container && context.containerUrl != nullptr) {
            add(KiriView::ImageDocumentEffect::prepareFailedContainer(*context.containerUrl));
        }
    }

    KiriView::ImageDocumentState &m_state;
    KiriView::ImageDocumentState::ChangeBatch m_batch;
    KiriView::ImageDocumentEffects m_effects;
};
}

namespace KiriView {
ImageOpenSourceLoadPlan ImageOpenWorkflow::sourceLoadPlan(const ImageOpenSourceLoadRequest &request)
{
    return imageOpenSourceLoadPlan(
        rustImageOpenSourceLoadPlan(RustImageOpenSourceLoadRequest { request.sourceUrlChanged,
            request.preserveTwoPageSpreadTransition, request.resetRightToLeftReading,
            request.rightToLeftReadingEnabled, request.containerNavigationUrlEmpty }));
}

ImageDocumentEffects ImageOpenWorkflow::beginSourceLoad(ImageDocumentState &state, bool hasImage)
{
    ImageOpenTransition transition(state);
    transition.applyBeginSourceLoad(
        rustImageOpenBeginSourceLoad(RustImageOpenBeginSourceLoadRequest {
            hasImage, state.loadingContainerNavigationUrl().isEmpty() }));
    return transition.takeEffects();
}

ImageDocumentEffects ImageOpenWorkflow::finishEmptySourceLoad(ImageDocumentState &state)
{
    ImageOpenTransition transition(state);
    transition.applyFinishEmptySourceLoad(rustImageOpenFinishEmptySourceLoad());
    return transition.takeEffects();
}

ImageDocumentEffects ImageOpenWorkflow::finishSuccessfulImageLoad(
    ImageDocumentState &state, const ImageLoadSession &session)
{
    ImageOpenTransition transition(state);
    transition.applyFinishSuccessfulImageLoad(
        rustImageOpenFinishSuccessfulImageLoad(RustImageOpenSuccessfulImageLoadRequest {
            session.request.containerNavigationUrl().isEmpty() }),
        transitionContextForSuccessfulImageLoad(session));
    return transition.takeEffects();
}

ImageDocumentEffects ImageOpenWorkflow::finishLoadWithError(ImageDocumentState &state,
    const ImageLoadSession &session, bool hasImage, const QString &errorString)
{
    ImageOpenTransition transition(state);
    const QUrl containerUrl = session.request.containerNavigationUrl();
    const QUrl displayedUrl = state.displayedUrl();
    transition.applyFinishLoadWithError(
        rustImageOpenFinishLoadWithError(
            sourceLoadErrorRequest(containerUrl.isEmpty(), hasImage, displayedUrl.isEmpty())),
        transitionContextForLoadError(session, containerUrl, displayedUrl, errorString));
    return transition.takeEffects();
}

ImageDocumentEffects ImageOpenWorkflow::finishContainerNavigationLoadWithError(
    ImageDocumentState &state, const QUrl &containerUrl, const QString &errorString)
{
    ImageOpenTransition transition(state);
    transition.applyFinishLoadWithError(rustImageOpenFinishLoadWithError(loadErrorRequest(
                                            RustImageOpenLoadErrorKind::ContainerNavigation)),
        transitionContextForContainerNavigationError(containerUrl, errorString));
    return transition.takeEffects();
}

ImageDocumentEffects ImageOpenWorkflow::finishAnimationLoadWithError(
    ImageDocumentState &state, const QString &errorString)
{
    ImageOpenTransition transition(state);
    transition.applyFinishLoadWithError(
        rustImageOpenFinishLoadWithError(loadErrorRequest(RustImageOpenLoadErrorKind::Animation)),
        transitionContextForAnimationError(errorString));
    return transition.takeEffects();
}
}
