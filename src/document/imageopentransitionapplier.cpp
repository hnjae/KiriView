// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopentransitionapplier.h"

#include "imagedocumentstate.h"
#include "location/imagedocumentlocation.h"

#include <iterator>
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

std::optional<KiriView::EmbeddedMetadata> embeddedMetadataTarget(
    KiriView::ImageOpenEmbeddedMetadataTarget target,
    const KiriView::ImageOpenTransitionContext &context)
{
    switch (target) {
    case KiriView::ImageOpenEmbeddedMetadataTarget::Clear:
        return KiriView::EmbeddedMetadata {};
    case KiriView::ImageOpenEmbeddedMetadataTarget::Provided:
        return context.embeddedMetadata;
    case KiriView::ImageOpenEmbeddedMetadataTarget::Unchanged:
        break;
    }

    return std::nullopt;
}

std::optional<KiriView::ImageDocumentPageKind> sourceKindTarget(
    KiriView::ImageOpenSourceKindTarget target, const KiriView::ImageOpenTransitionContext &context)
{
    switch (target) {
    case KiriView::ImageOpenSourceKindTarget::Session:
        if (context.session != nullptr) {
            return context.session->kind();
        }
        return std::nullopt;
    case KiriView::ImageOpenSourceKindTarget::Unchanged:
        return std::nullopt;
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

void appendRuntimeOperationsForOpenEffect(KiriView::ImageDocumentRuntimePlan &plan,
    KiriView::ImageOpenEffect effect, const KiriView::ImageOpenTransitionContext &context)
{
    switch (effect) {
    case KiriView::ImageOpenEffect::ClearImage: {
        KiriView::ImageDocumentRuntimePlan clearPlan = KiriView::imageDocumentClearImagePlan();
        plan.insert(plan.end(), std::make_move_iterator(clearPlan.begin()),
            std::make_move_iterator(clearPlan.end()));
        return;
    }
    case KiriView::ImageOpenEffect::ClearLoadingPresentation: {
        KiriView::ImageDocumentRuntimePlan clearPlan
            = KiriView::imageDocumentClearLoadingPresentationPlan();
        plan.insert(plan.end(), std::make_move_iterator(clearPlan.begin()),
            std::make_move_iterator(clearPlan.end()));
        return;
    }
    case KiriView::ImageOpenEffect::ResetZoom:
        plan.push_back(KiriView::ResetZoomOperation {});
        return;
    case KiriView::ImageOpenEffect::UpdatePageNavigation:
        plan.push_back(KiriView::UpdatePageNavigationOperation {});
        return;
    case KiriView::ImageOpenEffect::ScheduleAdjacentImagePredecode:
        plan.push_back(KiriView::ScheduleAdjacentImagePredecodeOperation {});
        return;
    case KiriView::ImageOpenEffect::PrepareFailedContainer:
        if (context.containerUrl.has_value()) {
            plan.push_back(KiriView::PrepareFailedContainerOperation { *context.containerUrl });
        }
        return;
    case KiriView::ImageOpenEffect::FinishSpreadTransition:
        plan.push_back(KiriView::FinishSpreadTransitionOperation {});
        return;
    case KiriView::ImageOpenEffect::ClearSecondaryPage:
        plan.push_back(KiriView::ClearSecondaryPageOperation {});
        return;
    }
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
        sourceKindTarget(delta.sourceKind, context),
        displayedLocationTarget(delta.displayedLocation, context),
        resolvedUrlForTarget(delta.containerNavigationUrl, context),
        boolTarget(delta.loading),
        documentStatus(delta.status),
        errorStringTarget(delta.errorString, context),
        boolTarget(delta.unsupportedOpenedCollectionVideo),
        embeddedMetadataTarget(delta.embeddedMetadata, context),
        delta.clearLoadingContainerNavigationUrl,
    };
}

KiriView::ImageDocumentRuntimePlan resolvedRuntimePlan(
    const KiriView::ImageOpenTransition &transition,
    const KiriView::ImageOpenTransitionContext &context)
{
    KiriView::ImageDocumentRuntimePlan plan;
    plan.reserve(transition.effects.size());
    for (KiriView::ImageOpenEffect effect : transition.effects) {
        appendRuntimeOperationsForOpenEffect(plan, effect, context);
    }
    return plan;
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
        m_runtimePlan = std::move(plan.runtimePlan);
    }

    KiriView::ImageDocumentRuntimePlan takeRuntimePlan() { return std::move(m_runtimePlan); }

private:
    void applyStateDelta(const KiriView::ImageOpenResolvedStateDelta &delta)
    {
        applyUnsupportedOpenedCollectionVideo(delta.unsupportedOpenedCollectionVideo, false);

        if (trackedLoadCompletionBeforeVisibleState(delta)) {
            applyTrackedLoadCompletion(delta);
            applyContainerNavigationUrl(delta.containerNavigationUrl);
            applySourceUrl(delta.sourceUrl);
            applySourceKind(delta.sourceKind);
            applyDisplayedLocation(delta.displayedLocation);
            applyErrorString(delta.errorString);
            applyEmbeddedMetadata(delta.embeddedMetadata);
            applyStatus(delta.status);
            applyUnsupportedOpenedCollectionVideo(delta.unsupportedOpenedCollectionVideo, true);
            return;
        }

        applySourceUrl(delta.sourceUrl);
        applySourceKind(delta.sourceKind);
        applyDisplayedLocation(delta.displayedLocation);
        applyContainerNavigationUrl(delta.containerNavigationUrl);
        applyErrorString(delta.errorString);
        applyEmbeddedMetadata(delta.embeddedMetadata);
        if (delta.clearLoadingContainerNavigationUrl) {
            applyTrackedLoadCompletion(delta);
        } else {
            applyLoading(delta.loading);
        }
        applyStatus(delta.status);
        applyUnsupportedOpenedCollectionVideo(delta.unsupportedOpenedCollectionVideo, true);
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

    void applySourceKind(const std::optional<KiriView::ImageDocumentPageKind> &sourceKind)
    {
        if (sourceKind.has_value()) {
            m_state.setSourceKind(*sourceKind);
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

    void applyEmbeddedMetadata(const std::optional<KiriView::EmbeddedMetadata> &metadata)
    {
        if (metadata.has_value()) {
            m_state.setEmbeddedMetadata(*metadata);
        }
    }

    void applyUnsupportedOpenedCollectionVideo(
        const std::optional<bool> &unsupported, bool targetValue)
    {
        if (unsupported.has_value() && *unsupported == targetValue) {
            m_state.setUnsupportedOpenedCollectionVideo(*unsupported);
        }
    }

    KiriView::ImageDocumentState &m_state;
    KiriView::ImageDocumentState::ChangeBatch m_batch;
    KiriView::ImageDocumentRuntimePlan m_runtimePlan;
};
}

namespace KiriView {
ImageOpenTransitionContext ImageOpenTransitionContext::sourceResolved(
    const ImageLoadSession &session)
{
    ImageOpenTransitionContext context;
    context.session = &session;
    return context;
}

ImageOpenTransitionContext ImageOpenTransitionContext::successfulImageLoad(
    const ImageLoadSession &session)
{
    ImageOpenTransitionContext context;
    context.session = &session;
    return context;
}

ImageOpenTransitionContext ImageOpenTransitionContext::successfulImageLoad(
    const ImageLoadSession &session, EmbeddedMetadata metadata)
{
    ImageOpenTransitionContext context = successfulImageLoad(session);
    context.embeddedMetadata = std::move(metadata);
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
        resolvedRuntimePlan(transition, context),
    };
}

ImageDocumentRuntimePlan applyImageOpenApplicationPlan(
    ImageDocumentState &state, ImageOpenApplicationPlan plan)
{
    ImageOpenTransitionApplier applier(state);
    applier.apply(std::move(plan));
    return applier.takeRuntimePlan();
}

ImageDocumentRuntimePlan applyImageOpenTransition(ImageDocumentState &state,
    const ImageOpenTransition &transition, const ImageOpenTransitionContext &context)
{
    return applyImageOpenApplicationPlan(state, imageOpenApplicationPlan(transition, context));
}
}
