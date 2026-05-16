// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflow.h"

#include "imagecontainer.h"
#include "imagedocumentsourceloadrequest.h"
#include "imagedocumentstate.h"
#include "imagespreadpresentationcontroller.h"
#include "kiriview/src/imageopenworkflow.cxx.h"

#include <optional>
#include <utility>

namespace {
KiriView::RustImageDocumentSourceLoadKind rustSourceLoadKind(
    KiriView::ImageDocumentSourceLoadKind loadKind)
{
    switch (loadKind) {
    case KiriView::ImageDocumentSourceLoadKind::CurrentSource:
        return KiriView::RustImageDocumentSourceLoadKind::CurrentSource;
    case KiriView::ImageDocumentSourceLoadKind::ReplacementSource:
        return KiriView::RustImageDocumentSourceLoadKind::ReplacementSource;
    }

    return KiriView::RustImageDocumentSourceLoadKind::CurrentSource;
}

KiriView::RustImageDocumentRightToLeftReadingReset rustRightToLeftReadingReset(
    KiriView::ImageDocumentRightToLeftReadingReset reset)
{
    switch (reset) {
    case KiriView::ImageDocumentRightToLeftReadingReset::Keep:
        return KiriView::RustImageDocumentRightToLeftReadingReset::Keep;
    case KiriView::ImageDocumentRightToLeftReadingReset::ResetInactive:
        return KiriView::RustImageDocumentRightToLeftReadingReset::ResetInactive;
    case KiriView::ImageDocumentRightToLeftReadingReset::ResetActive:
        return KiriView::RustImageDocumentRightToLeftReadingReset::ResetActive;
    }

    return KiriView::RustImageDocumentRightToLeftReadingReset::Keep;
}

KiriView::RustImageDocumentSourceLoadPolicyInput rustSourceLoadPolicyInput(
    const KiriView::ImageDocumentSourceLoadPolicyInput &input)
{
    KiriView::RustImageDocumentSourceLoadPolicyInput rustInput {};
    rustInput.load_kind = rustSourceLoadKind(input.loadKind);
    rustInput.preserve_two_page_spread_transition = input.preserveTwoPageSpreadTransition;
    rustInput.right_to_left_reading_reset
        = rustRightToLeftReadingReset(input.rightToLeftReadingReset);
    rustInput.has_requested_container_navigation_url = input.hasRequestedContainerNavigationUrl;
    return rustInput;
}

KiriView::ImageDocumentRightToLeftReadingTransition rightToLeftReadingTransition(
    KiriView::RustImageDocumentRightToLeftReadingTransition transition)
{
    switch (transition) {
    case KiriView::RustImageDocumentRightToLeftReadingTransition::Keep:
        return KiriView::ImageDocumentRightToLeftReadingTransition::Keep;
    case KiriView::RustImageDocumentRightToLeftReadingTransition::Reset:
        return KiriView::ImageDocumentRightToLeftReadingTransition::Reset;
    case KiriView::RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyBeforeSourceState:
        return KiriView::ImageDocumentRightToLeftReadingTransition::ResetAndNotifyBeforeSourceState;
    case KiriView::RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyAfterOpen:
        return KiriView::ImageDocumentRightToLeftReadingTransition::ResetAndNotifyAfterOpen;
    }

    return KiriView::ImageDocumentRightToLeftReadingTransition::Keep;
}

KiriView::ImageDocumentSourceLoadUrlTarget sourceLoadUrlTarget(
    KiriView::RustImageDocumentSourceLoadUrlTarget target)
{
    switch (target) {
    case KiriView::RustImageDocumentSourceLoadUrlTarget::Unchanged:
        return KiriView::ImageDocumentSourceLoadUrlTarget::Unchanged;
    case KiriView::RustImageDocumentSourceLoadUrlTarget::Empty:
        return KiriView::ImageDocumentSourceLoadUrlTarget::Empty;
    case KiriView::RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation:
        return KiriView::ImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation;
    case KiriView::RustImageDocumentSourceLoadUrlTarget::RequestedSource:
        return KiriView::ImageDocumentSourceLoadUrlTarget::RequestedSource;
    }

    return KiriView::ImageDocumentSourceLoadUrlTarget::Unchanged;
}

KiriView::ImageDocumentSourceLoadPlan imageDocumentSourceLoadPlan(
    const KiriView::RustImageDocumentSourceLoadPlan &rustPlan)
{
    return KiriView::ImageDocumentSourceLoadPlan {
        rustPlan.cancel_navigation_and_predecode,
        rustPlan.finish_spread_transition,
        rightToLeftReadingTransition(rustPlan.right_to_left_reading_transition),
        rustPlan.clear_secondary_page,
        sourceLoadUrlTarget(rustPlan.loading_container_navigation_url),
        sourceLoadUrlTarget(rustPlan.container_navigation_url),
        sourceLoadUrlTarget(rustPlan.source_url),
        rustPlan.begin_open,
    };
}

KiriView::ImageDocumentSourceLoadKind sourceLoadKind(const KiriView::ImageDocumentState &state,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    if (state.sourceUrl() == request.sourceUrl) {
        return KiriView::ImageDocumentSourceLoadKind::CurrentSource;
    }

    return KiriView::ImageDocumentSourceLoadKind::ReplacementSource;
}

KiriView::ImageDocumentRightToLeftReadingReset rightToLeftReadingReset(
    const KiriView::ImageDocumentState &state,
    const KiriView::ImageSpreadPresentationController &spreadController,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    if (!spreadController.shouldResetRightToLeftReadingForLoad(
            state.displayedArchiveDocument(), request.sourceUrl, request.containerNavigationUrl)) {
        return KiriView::ImageDocumentRightToLeftReadingReset::Keep;
    }

    return spreadController.rightToLeftReadingEnabled()
        ? KiriView::ImageDocumentRightToLeftReadingReset::ResetActive
        : KiriView::ImageDocumentRightToLeftReadingReset::ResetInactive;
}

KiriView::ImageDocumentSourceLoadPolicyInput sourceLoadPolicyInput(
    const KiriView::ImageDocumentState &state,
    const KiriView::ImageSpreadPresentationController &spreadController,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    KiriView::ImageDocumentSourceLoadPolicyInput input;
    input.loadKind = sourceLoadKind(state, request);
    input.preserveTwoPageSpreadTransition = request.preserveTwoPageSpreadTransition;
    input.rightToLeftReadingReset = rightToLeftReadingReset(state, spreadController, request);
    input.hasRequestedContainerNavigationUrl = !request.containerNavigationUrl.isEmpty();
    return input;
}

struct ImageOpenTransitionContext {
    const KiriView::ImageLoadSession *session = nullptr;
    std::optional<QUrl> containerUrl;
    std::optional<QUrl> displayedUrl;
    std::optional<QString> errorString;

    static ImageOpenTransitionContext successfulImageLoad(const KiriView::ImageLoadSession &session)
    {
        ImageOpenTransitionContext context;
        context.session = &session;
        return context;
    }

    static ImageOpenTransitionContext sourceLoadError(const KiriView::ImageLoadSession &session,
        const QUrl &displayedUrl, const QString &errorString)
    {
        ImageOpenTransitionContext context;
        context.session = &session;
        context.containerUrl = session.request.containerNavigationUrl();
        context.displayedUrl = displayedUrl;
        context.errorString = errorString;
        return context;
    }

    static ImageOpenTransitionContext containerNavigationError(
        const QUrl &containerUrl, const QString &errorString)
    {
        ImageOpenTransitionContext context;
        context.containerUrl = containerUrl;
        context.errorString = errorString;
        return context;
    }

    static ImageOpenTransitionContext animationError(const QString &errorString)
    {
        ImageOpenTransitionContext context;
        context.errorString = errorString;
        return context;
    }

    QUrl urlForTarget(KiriView::ImageOpenUrlTarget target) const
    {
        switch (target) {
        case KiriView::ImageOpenUrlTarget::Empty:
            return QUrl();
        case KiriView::ImageOpenUrlTarget::SessionImage:
            return session != nullptr ? session->location.imageUrl() : QUrl();
        case KiriView::ImageOpenUrlTarget::SessionContainerNavigation:
            return session != nullptr ? session->request.containerNavigationUrl() : QUrl();
        case KiriView::ImageOpenUrlTarget::DerivedContainerNavigation:
            return session != nullptr
                ? KiriView::containerNavigationUrlForLocation(session->location)
                : QUrl();
        case KiriView::ImageOpenUrlTarget::Container:
            return containerUrl.value_or(QUrl());
        case KiriView::ImageOpenUrlTarget::Displayed:
            return displayedUrl.value_or(QUrl());
        case KiriView::ImageOpenUrlTarget::Unchanged:
            break;
        }

        return QUrl();
    }

    const KiriView::DisplayedImageLocation *sessionLocation() const
    {
        return session != nullptr ? &session->location : nullptr;
    }

    QString providedErrorString() const { return errorString.value_or(QString()); }
};

KiriView::ImageOpenWorkflowEvent imageOpenWorkflowEvent(KiriView::ImageOpenWorkflowEventKind kind)
{
    KiriView::ImageOpenWorkflowEvent event {};
    event.kind = kind;
    return event;
}

KiriView::ImageOpenWorkflowEvent beginSourceLoadEvent(
    bool hasImage, bool loadingContainerNavigationUrlEmpty)
{
    KiriView::ImageOpenWorkflowEvent event
        = imageOpenWorkflowEvent(KiriView::ImageOpenWorkflowEventKind::BeginSourceLoad);
    event.has_image = hasImage;
    event.loading_container_navigation_url_empty = loadingContainerNavigationUrlEmpty;
    return event;
}

KiriView::ImageOpenWorkflowEvent successfulImageLoadEvent(const KiriView::ImageLoadSession &session)
{
    KiriView::ImageOpenWorkflowEvent event
        = imageOpenWorkflowEvent(KiriView::ImageOpenWorkflowEventKind::FinishSuccessfulImageLoad);
    event.request_container_navigation_url_empty
        = session.request.containerNavigationUrl().isEmpty();
    return event;
}

KiriView::ImageOpenWorkflowEvent sourceLoadErrorEvent(
    bool containerNavigationUrlEmpty, bool hasImage, bool displayedUrlEmpty)
{
    KiriView::ImageOpenWorkflowEvent event
        = imageOpenWorkflowEvent(KiriView::ImageOpenWorkflowEventKind::FinishSourceLoadWithError);
    event.container_navigation_url_empty = containerNavigationUrlEmpty;
    event.has_image = hasImage;
    event.displayed_url_empty = displayedUrlEmpty;
    return event;
}

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
    KiriView::ImageOpenEffect effect, const ImageOpenTransitionContext &context)
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

    void apply(
        const KiriView::ImageOpenTransition &transition, const ImageOpenTransitionContext &context)
    {
        for (const KiriView::ImageOpenTransitionOperation &operation : transition.operations) {
            applyOperation(operation, context);
        }
    }

    KiriView::ImageDocumentEffects takeEffects() { return std::move(m_effects); }

private:
    void applyOperation(const KiriView::ImageOpenTransitionOperation &operation,
        const ImageOpenTransitionContext &context)
    {
        switch (operation.kind) {
        case KiriView::ImageOpenTransitionOperationKind::SourceUrl:
            applySourceUrlTarget(operation.url_target, context);
            return;
        case KiriView::ImageOpenTransitionOperationKind::DisplayedLocation:
            applyDisplayedLocationTarget(operation.displayed_location_target, context);
            return;
        case KiriView::ImageOpenTransitionOperationKind::ContainerNavigationUrl:
            applyContainerNavigationUrlTarget(operation.url_target, context);
            return;
        case KiriView::ImageOpenTransitionOperationKind::Loading:
            applyLoadingTarget(operation.bool_target);
            return;
        case KiriView::ImageOpenTransitionOperationKind::Status:
            applyStatusTarget(operation.status_target);
            return;
        case KiriView::ImageOpenTransitionOperationKind::ErrorString:
            applyErrorStringTarget(operation.error_string_target, context);
            return;
        case KiriView::ImageOpenTransitionOperationKind::ClearLoadingContainerNavigationUrl:
            m_state.clearLoadingContainerNavigationUrl();
            return;
        case KiriView::ImageOpenTransitionOperationKind::Effect:
            applyEffect(operation.effect, context);
            return;
        }
    }

    void applySourceUrlTarget(
        KiriView::ImageOpenUrlTarget target, const ImageOpenTransitionContext &context)
    {
        if (target != KiriView::ImageOpenUrlTarget::Unchanged) {
            m_state.setSourceUrl(context.urlForTarget(target));
        }
    }

    void applyDisplayedLocationTarget(KiriView::ImageOpenDisplayedLocationTarget target,
        const ImageOpenTransitionContext &context)
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
        KiriView::ImageOpenUrlTarget target, const ImageOpenTransitionContext &context)
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

    void applyErrorStringTarget(
        KiriView::ImageOpenErrorStringTarget target, const ImageOpenTransitionContext &context)
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

    void applyEffect(KiriView::ImageOpenEffect effect, const ImageOpenTransitionContext &context)
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

namespace KiriView::ImageOpenWorkflow {
ImageDocumentSourceLoadPlan sourceLoadPlan(const ImageDocumentSourceLoadPolicyInput &input)
{
    return imageDocumentSourceLoadPlan(
        rustImageDocumentSourceLoadPlan(rustSourceLoadPolicyInput(input)));
}

ImageDocumentSourceLoadPlan sourceLoadPlan(const ImageDocumentState &state,
    const ImageSpreadPresentationController &spreadController,
    const ImageDocumentSourceLoadRequest &request)
{
    return sourceLoadPlan(::sourceLoadPolicyInput(state, spreadController, request));
}

ImageDocumentEffects beginSourceLoad(ImageDocumentState &state, bool hasImage)
{
    ImageOpenTransitionApplier transition(state);
    transition.apply(rustImageOpenTransition(beginSourceLoadEvent(
                         hasImage, state.loadingContainerNavigationUrl().isEmpty())),
        ImageOpenTransitionContext());
    return transition.takeEffects();
}

ImageDocumentEffects finishEmptySourceLoad(ImageDocumentState &state)
{
    ImageOpenTransitionApplier transition(state);
    transition.apply(rustImageOpenTransition(
                         imageOpenWorkflowEvent(ImageOpenWorkflowEventKind::FinishEmptySourceLoad)),
        ImageOpenTransitionContext());
    return transition.takeEffects();
}

ImageDocumentEffects finishSuccessfulImageLoad(
    ImageDocumentState &state, const ImageLoadSession &session)
{
    ImageOpenTransitionApplier transition(state);
    transition.apply(rustImageOpenTransition(successfulImageLoadEvent(session)),
        ImageOpenTransitionContext::successfulImageLoad(session));
    return transition.takeEffects();
}

ImageDocumentEffects finishLoadWithError(ImageDocumentState &state, const ImageLoadSession &session,
    bool hasImage, const QString &errorString)
{
    ImageOpenTransitionApplier transition(state);
    const QUrl containerUrl = session.request.containerNavigationUrl();
    const QUrl displayedUrl = state.displayedUrl();
    transition.apply(rustImageOpenTransition(sourceLoadErrorEvent(
                         containerUrl.isEmpty(), hasImage, displayedUrl.isEmpty())),
        ImageOpenTransitionContext::sourceLoadError(session, displayedUrl, errorString));
    return transition.takeEffects();
}

ImageDocumentEffects finishContainerNavigationLoadWithError(
    ImageDocumentState &state, const QUrl &containerUrl, const QString &errorString)
{
    ImageOpenTransitionApplier transition(state);
    transition.apply(rustImageOpenTransition(imageOpenWorkflowEvent(
                         ImageOpenWorkflowEventKind::FinishContainerNavigationLoadWithError)),
        ImageOpenTransitionContext::containerNavigationError(containerUrl, errorString));
    return transition.takeEffects();
}

ImageDocumentEffects finishAnimationLoadWithError(
    ImageDocumentState &state, const QString &errorString)
{
    ImageOpenTransitionApplier transition(state);
    transition.apply(rustImageOpenTransition(imageOpenWorkflowEvent(
                         ImageOpenWorkflowEventKind::FinishAnimationLoadWithError)),
        ImageOpenTransitionContext::animationError(errorString));
    return transition.takeEffects();
}
}
