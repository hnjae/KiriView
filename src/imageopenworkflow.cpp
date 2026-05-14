// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflow.h"

#include "imagecontainer.h"
#include "imagedocumentstate.h"
#include "kiriview/src/imageopenworkflow.cxx.h"

#include <optional>
#include <utility>

namespace {
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

    QUrl urlForTarget(KiriView::RustImageOpenUrlTarget target) const
    {
        switch (target) {
        case KiriView::RustImageOpenUrlTarget::Empty:
            return QUrl();
        case KiriView::RustImageOpenUrlTarget::SessionImage:
            return session != nullptr ? session->location.imageUrl() : QUrl();
        case KiriView::RustImageOpenUrlTarget::SessionContainerNavigation:
            return session != nullptr ? session->request.containerNavigationUrl() : QUrl();
        case KiriView::RustImageOpenUrlTarget::DerivedContainerNavigation:
            return session != nullptr
                ? KiriView::containerNavigationUrlForLocation(session->location)
                : QUrl();
        case KiriView::RustImageOpenUrlTarget::Container:
            return containerUrl.value_or(QUrl());
        case KiriView::RustImageOpenUrlTarget::Displayed:
            return displayedUrl.value_or(QUrl());
        case KiriView::RustImageOpenUrlTarget::Unchanged:
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

KiriView::RustImageOpenLoadErrorRequest sourceLoadErrorRequest(
    bool containerNavigationUrlEmpty, bool hasImage, bool displayedUrlEmpty)
{
    KiriView::RustImageOpenLoadErrorRequest request {};
    request.kind = KiriView::RustImageOpenLoadErrorKind::SourceLoad;
    request.container_navigation_url_empty = containerNavigationUrlEmpty;
    request.has_image = hasImage;
    request.displayed_url_empty = displayedUrlEmpty;
    return request;
}

KiriView::RustImageOpenLoadErrorRequest loadErrorRequest(KiriView::RustImageOpenLoadErrorKind kind)
{
    KiriView::RustImageOpenLoadErrorRequest request {};
    request.kind = kind;
    request.container_navigation_url_empty = true;
    request.has_image = false;
    request.displayed_url_empty = true;
    return request;
}

KiriView::RustImageOpenBeginSourceLoadRequest rustImageOpenBeginSourceLoadRequest(
    bool hasImage, bool loadingContainerNavigationUrlEmpty)
{
    KiriView::RustImageOpenBeginSourceLoadRequest request {};
    request.has_image = hasImage;
    request.loading_container_navigation_url_empty = loadingContainerNavigationUrlEmpty;
    return request;
}

KiriView::RustImageOpenSuccessfulImageLoadRequest rustImageOpenSuccessfulImageLoadRequest(
    const KiriView::ImageLoadSession &session)
{
    KiriView::RustImageOpenSuccessfulImageLoadRequest request {};
    request.request_container_navigation_url_empty
        = session.request.containerNavigationUrl().isEmpty();
    return request;
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

std::optional<KiriView::ImageDocumentEffect> imageOpenEffect(
    KiriView::RustImageOpenEffect effect, const ImageOpenTransitionContext &context)
{
    switch (effect) {
    case KiriView::RustImageOpenEffect::ClearImage:
        return KiriView::ImageDocumentEffect::clearImage();
    case KiriView::RustImageOpenEffect::ResetZoom:
        return KiriView::ImageDocumentEffect::resetZoom();
    case KiriView::RustImageOpenEffect::UpdatePageNavigation:
        return KiriView::ImageDocumentEffect::updatePageNavigation();
    case KiriView::RustImageOpenEffect::ScheduleAdjacentImagePredecode:
        return KiriView::ImageDocumentEffect::scheduleAdjacentImagePredecode();
    case KiriView::RustImageOpenEffect::PrepareFailedContainer:
        if (context.containerUrl.has_value()) {
            return KiriView::ImageDocumentEffect::prepareFailedContainer(*context.containerUrl);
        }
        return std::nullopt;
    }

    return std::nullopt;
}

void appendImageOpenEffects(KiriView::ImageDocumentEffects *documentEffects,
    const rust::Vec<KiriView::RustImageOpenEffect> &effects,
    const ImageOpenTransitionContext &context)
{
    for (KiriView::RustImageOpenEffect effect : effects) {
        std::optional<KiriView::ImageDocumentEffect> documentEffect
            = imageOpenEffect(effect, context);
        if (documentEffect.has_value()) {
            documentEffects->push_back(std::move(*documentEffect));
        }
    }
}

class ImageOpenTransitionApplier final
{
public:
    explicit ImageOpenTransitionApplier(KiriView::ImageDocumentState &state)
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
            m_state.setSourceUrl(context.urlForTarget(target));
        }
    }

    void applyDisplayedLocationTarget(KiriView::RustImageOpenDisplayedLocationTarget target,
        const ImageOpenTransitionContext &context)
    {
        const KiriView::DisplayedImageLocation *location = context.sessionLocation();
        if (target == KiriView::RustImageOpenDisplayedLocationTarget::SessionLocation
            && location != nullptr) {
            m_state.setDisplayedImageLocation(*location);
        }
    }

    void applyContainerNavigationUrlTarget(
        KiriView::RustImageOpenUrlTarget target, const ImageOpenTransitionContext &context)
    {
        if (target != KiriView::RustImageOpenUrlTarget::Unchanged) {
            m_state.setContainerNavigationUrl(context.urlForTarget(target));
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
            m_state.setErrorString(context.providedErrorString());
            return;
        case KiriView::RustImageOpenErrorStringTarget::Unchanged:
            return;
        }
    }

    void applyEffects(const rust::Vec<KiriView::RustImageOpenEffect> &effects,
        const ImageOpenTransitionContext &context)
    {
        appendImageOpenEffects(&m_effects, effects, context);
    }

    KiriView::ImageDocumentState &m_state;
    KiriView::ImageDocumentState::ChangeBatch m_batch;
    KiriView::ImageDocumentEffects m_effects;
};
}

namespace KiriView::ImageOpenWorkflow {
ImageDocumentEffects beginSourceLoad(ImageDocumentState &state, bool hasImage)
{
    ImageOpenTransitionApplier transition(state);
    transition.applyBeginSourceLoad(
        rustImageOpenBeginSourceLoad(rustImageOpenBeginSourceLoadRequest(
            hasImage, state.loadingContainerNavigationUrl().isEmpty())));
    return transition.takeEffects();
}

ImageDocumentEffects finishEmptySourceLoad(ImageDocumentState &state)
{
    ImageOpenTransitionApplier transition(state);
    transition.applyFinishEmptySourceLoad(rustImageOpenFinishEmptySourceLoad());
    return transition.takeEffects();
}

ImageDocumentEffects finishSuccessfulImageLoad(
    ImageDocumentState &state, const ImageLoadSession &session)
{
    ImageOpenTransitionApplier transition(state);
    transition.applyFinishSuccessfulImageLoad(
        rustImageOpenFinishSuccessfulImageLoad(rustImageOpenSuccessfulImageLoadRequest(session)),
        ImageOpenTransitionContext::successfulImageLoad(session));
    return transition.takeEffects();
}

ImageDocumentEffects finishLoadWithError(ImageDocumentState &state, const ImageLoadSession &session,
    bool hasImage, const QString &errorString)
{
    ImageOpenTransitionApplier transition(state);
    const QUrl containerUrl = session.request.containerNavigationUrl();
    const QUrl displayedUrl = state.displayedUrl();
    transition.applyFinishLoadWithError(
        rustImageOpenFinishLoadWithError(
            sourceLoadErrorRequest(containerUrl.isEmpty(), hasImage, displayedUrl.isEmpty())),
        ImageOpenTransitionContext::sourceLoadError(session, displayedUrl, errorString));
    return transition.takeEffects();
}

ImageDocumentEffects finishContainerNavigationLoadWithError(
    ImageDocumentState &state, const QUrl &containerUrl, const QString &errorString)
{
    ImageOpenTransitionApplier transition(state);
    transition.applyFinishLoadWithError(rustImageOpenFinishLoadWithError(loadErrorRequest(
                                            RustImageOpenLoadErrorKind::ContainerNavigation)),
        ImageOpenTransitionContext::containerNavigationError(containerUrl, errorString));
    return transition.takeEffects();
}

ImageDocumentEffects finishAnimationLoadWithError(
    ImageDocumentState &state, const QString &errorString)
{
    ImageOpenTransitionApplier transition(state);
    transition.applyFinishLoadWithError(
        rustImageOpenFinishLoadWithError(loadErrorRequest(RustImageOpenLoadErrorKind::Animation)),
        ImageOpenTransitionContext::animationError(errorString));
    return transition.takeEffects();
}
}
