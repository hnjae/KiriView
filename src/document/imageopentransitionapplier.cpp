// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopentransitionapplier.h"

#include "imagedocumentstate.h"
#include "location/imagedocumentlocation.h"

#include <iterator>
#include <utility>

namespace {
std::optional<kiriview::ImageDocumentStatus> documentStatus(kiriview::ImageOpenStatusTarget status)
{
    switch (status) {
    case kiriview::ImageOpenStatusTarget::Null:
        return kiriview::ImageDocumentStatus::Null;
    case kiriview::ImageOpenStatusTarget::Loading:
        return kiriview::ImageDocumentStatus::Loading;
    case kiriview::ImageOpenStatusTarget::Ready:
        return kiriview::ImageDocumentStatus::Ready;
    case kiriview::ImageOpenStatusTarget::Error:
        return kiriview::ImageDocumentStatus::Error;
    case kiriview::ImageOpenStatusTarget::Unchanged:
        break;
    }

    return std::nullopt;
}

std::optional<bool> boolTarget(kiriview::ImageOpenBoolTarget target)
{
    switch (target) {
    case kiriview::ImageOpenBoolTarget::False:
        return false;
    case kiriview::ImageOpenBoolTarget::True:
        return true;
    case kiriview::ImageOpenBoolTarget::Unchanged:
        break;
    }

    return std::nullopt;
}

std::optional<QString> errorStringTarget(kiriview::ImageOpenErrorStringTarget target,
    const kiriview::ImageOpenTransitionContext &context)
{
    switch (target) {
    case kiriview::ImageOpenErrorStringTarget::Clear:
        return QString();
    case kiriview::ImageOpenErrorStringTarget::Provided:
        return context.errorString;
    case kiriview::ImageOpenErrorStringTarget::Unchanged:
        break;
    }

    return std::nullopt;
}

std::optional<kiriview::ImageLoadFailure> loadFailureTarget(
    kiriview::ImageOpenErrorStringTarget target,
    const kiriview::ImageOpenTransitionContext &context)
{
    if (target == kiriview::ImageOpenErrorStringTarget::Provided) {
        return context.loadFailure;
    }

    return std::nullopt;
}

std::optional<kiriview::EmbeddedMetadata> embeddedMetadataTarget(
    kiriview::ImageOpenEmbeddedMetadataTarget target,
    const kiriview::ImageOpenTransitionContext &context)
{
    switch (target) {
    case kiriview::ImageOpenEmbeddedMetadataTarget::Clear:
        return kiriview::EmbeddedMetadata {};
    case kiriview::ImageOpenEmbeddedMetadataTarget::Provided:
        return context.embeddedMetadata;
    case kiriview::ImageOpenEmbeddedMetadataTarget::Unchanged:
        break;
    }

    return std::nullopt;
}

std::optional<kiriview::ImageDocumentPageKind> sourceKindTarget(
    kiriview::ImageOpenSourceKindTarget target, const kiriview::ImageOpenTransitionContext &context)
{
    switch (target) {
    case kiriview::ImageOpenSourceKindTarget::Session:
        if (context.session != nullptr) {
            return context.session->kind();
        }
        return std::nullopt;
    case kiriview::ImageOpenSourceKindTarget::Unchanged:
        return std::nullopt;
    }

    return std::nullopt;
}

std::optional<kiriview::DisplayedImageLocation> displayedLocationTarget(
    kiriview::ImageOpenDisplayedLocationTarget target,
    const kiriview::ImageOpenTransitionContext &context)
{
    switch (target) {
    case kiriview::ImageOpenDisplayedLocationTarget::Session:
        if (context.session != nullptr) {
            return context.session->location();
        }
        return std::nullopt;
    case kiriview::ImageOpenDisplayedLocationTarget::Unchanged:
        return std::nullopt;
    }

    return std::nullopt;
}

void appendRuntimeOperationsForOpenEffect(kiriview::ImageDocumentRuntimePlan &plan,
    kiriview::ImageOpenEffect effect, const kiriview::ImageOpenTransitionContext &context)
{
    switch (effect) {
    case kiriview::ImageOpenEffect::ClearImage: {
        kiriview::ImageDocumentRuntimePlan clearPlan = kiriview::imageDocumentClearImagePlan();
        plan.insert(plan.end(), std::make_move_iterator(clearPlan.begin()),
            std::make_move_iterator(clearPlan.end()));
        return;
    }
    case kiriview::ImageOpenEffect::ClearLoadingPresentation: {
        kiriview::ImageDocumentRuntimePlan clearPlan
            = kiriview::imageDocumentClearLoadingPresentationPlan();
        plan.insert(plan.end(), std::make_move_iterator(clearPlan.begin()),
            std::make_move_iterator(clearPlan.end()));
        return;
    }
    case kiriview::ImageOpenEffect::ResetZoom:
        plan.push_back(kiriview::ResetZoomOperation {});
        return;
    case kiriview::ImageOpenEffect::UpdatePageNavigation:
        plan.push_back(kiriview::UpdatePageNavigationOperation {});
        return;
    case kiriview::ImageOpenEffect::ScheduleAdjacentImagePredecode:
        plan.push_back(kiriview::ScheduleAdjacentImagePredecodeOperation {});
        return;
    case kiriview::ImageOpenEffect::PrepareFailedContainer:
        if (context.containerUrl.has_value()) {
            plan.push_back(kiriview::PrepareFailedContainerOperation { *context.containerUrl });
        }
        return;
    case kiriview::ImageOpenEffect::FinishSpreadTransition:
        plan.push_back(kiriview::FinishSpreadTransitionOperation {});
        return;
    case kiriview::ImageOpenEffect::ClearSecondaryPage:
        plan.push_back(kiriview::ClearSecondaryPageOperation {});
        return;
    }
}

std::optional<QUrl> resolvedUrlForTarget(
    kiriview::ImageOpenUrlTarget target, const kiriview::ImageOpenTransitionContext &context)
{
    switch (target) {
    case kiriview::ImageOpenUrlTarget::Empty:
        return QUrl();
    case kiriview::ImageOpenUrlTarget::SessionImage:
        return context.session != nullptr ? std::optional<QUrl>(context.session->imageUrl())
                                          : std::nullopt;
    case kiriview::ImageOpenUrlTarget::SessionContainerNavigation:
        return context.session != nullptr
            ? std::optional<QUrl>(context.session->containerNavigationUrl())
            : std::nullopt;
    case kiriview::ImageOpenUrlTarget::DerivedContainerNavigation:
        return context.session != nullptr
            ? std::optional<QUrl>(containerNavigationUrlForLocation(context.session->location()))
            : std::nullopt;
    case kiriview::ImageOpenUrlTarget::Container:
        return context.containerUrl;
    case kiriview::ImageOpenUrlTarget::Displayed:
        return context.displayedUrl;
    case kiriview::ImageOpenUrlTarget::Unchanged:
        break;
    }

    return std::nullopt;
}

kiriview::ImageOpenResolvedStateDelta resolvedStateDelta(
    const kiriview::ImageOpenStateDelta &delta, const kiriview::ImageOpenTransitionContext &context)
{
    return kiriview::ImageOpenResolvedStateDelta {
        resolvedUrlForTarget(delta.sourceUrl, context),
        sourceKindTarget(delta.sourceKind, context),
        displayedLocationTarget(delta.displayedLocation, context),
        resolvedUrlForTarget(delta.containerNavigationUrl, context),
        boolTarget(delta.loading),
        documentStatus(delta.status),
        errorStringTarget(delta.errorString, context),
        loadFailureTarget(delta.errorString, context),
        boolTarget(delta.unsupportedOpenedCollectionVideo),
        embeddedMetadataTarget(delta.embeddedMetadata, context),
        delta.clearLoadingContainerNavigationUrl,
    };
}

kiriview::ImageDocumentRuntimePlan resolvedRuntimePlan(
    const kiriview::ImageOpenTransition &transition,
    const kiriview::ImageOpenTransitionContext &context)
{
    kiriview::ImageDocumentRuntimePlan plan;
    plan.reserve(transition.effects.size());
    for (kiriview::ImageOpenEffect effect : transition.effects) {
        appendRuntimeOperationsForOpenEffect(plan, effect, context);
    }
    return plan;
}

QString finalErrorString(
    const kiriview::ImageDocumentState &state, const kiriview::ImageOpenResolvedStateDelta &delta)
{
    if (delta.loadFailure.has_value()) {
        return delta.loadFailure->userMessage;
    }
    if (delta.errorString.has_value()) {
        return *delta.errorString;
    }
    return state.errorString();
}

QUrl finalSourceUrl(
    const kiriview::ImageDocumentState &state, const kiriview::ImageOpenResolvedStateDelta &delta)
{
    return delta.sourceUrl.value_or(state.sourceUrl());
}

kiriview::DisplayedImageLocation finalDisplayedLocation(
    const kiriview::ImageDocumentState &state, const kiriview::ImageOpenResolvedStateDelta &delta)
{
    return delta.displayedLocation.value_or(state.displayedImageLocation());
}

kiriview::ImageDocumentPageKind finalSourceKind(
    const kiriview::ImageDocumentState &state, const kiriview::ImageOpenResolvedStateDelta &delta)
{
    return delta.sourceKind.value_or(state.sourceKind());
}

bool finalUnsupportedOpenedCollectionVideo(
    const kiriview::ImageDocumentState &state, const kiriview::ImageOpenResolvedStateDelta &delta)
{
    return delta.unsupportedOpenedCollectionVideo.value_or(
        state.unsupportedOpenedCollectionVideo());
}

QUrl finalContainerNavigationUrl(
    const kiriview::ImageDocumentState &state, const kiriview::ImageOpenResolvedStateDelta &delta)
{
    return delta.containerNavigationUrl.value_or(state.containerNavigationUrl());
}

bool finalImageOpenStateIsValid(
    const kiriview::ImageDocumentState &state, const kiriview::ImageOpenResolvedStateDelta &delta)
{
    const kiriview::ImageDocumentStatus status = delta.status.value_or(state.status());
    const bool loading = delta.loading.value_or(state.loading());
    const bool hasError = !finalErrorString(state, delta).isEmpty();

    switch (status) {
    case kiriview::ImageDocumentStatus::Null:
        return !loading && !hasError && finalContainerNavigationUrl(state, delta).isEmpty();
    case kiriview::ImageDocumentStatus::Loading:
        return loading && !hasError;
    case kiriview::ImageDocumentStatus::Ready:
        return !loading && !hasError && !finalSourceUrl(state, delta).isEmpty()
            && !finalDisplayedLocation(state, delta).isEmpty()
            && (finalSourceKind(state, delta) != kiriview::ImageDocumentPageKind::Video
                || finalUnsupportedOpenedCollectionVideo(state, delta));
    case kiriview::ImageDocumentStatus::Error:
        return !loading && hasError;
    }

    return false;
}

class ImageOpenTransitionApplier final
{
public:
    explicit ImageOpenTransitionApplier(kiriview::ImageDocumentState &state)
        : m_state(state)
        , m_batch(m_state.beginChangeBatch())
    {
    }

    void apply(kiriview::ImageOpenApplicationPlan plan)
    {
        if (!finalImageOpenStateIsValid(m_state, plan.stateDelta)) {
            return;
        }

        applyStateDelta(plan.stateDelta);
        m_runtimePlan = std::move(plan.runtimePlan);
    }

    kiriview::ImageDocumentRuntimePlan takeRuntimePlan() { return std::move(m_runtimePlan); }

private:
    void applyStateDelta(const kiriview::ImageOpenResolvedStateDelta &delta)
    {
        applyUnsupportedOpenedCollectionVideo(delta.unsupportedOpenedCollectionVideo, false);

        if (trackedLoadCompletionBeforeVisibleState(delta)) {
            applyTrackedLoadCompletion(delta);
            applyContainerNavigationUrl(delta.containerNavigationUrl);
            applySourceUrl(delta.sourceUrl);
            applySourceKind(delta.sourceKind);
            applyDisplayedLocation(delta.displayedLocation);
            applyError(delta.errorString, delta.loadFailure);
            applyEmbeddedMetadata(delta.embeddedMetadata);
            applyStatus(delta.status);
            applyUnsupportedOpenedCollectionVideo(delta.unsupportedOpenedCollectionVideo, true);
            return;
        }

        applySourceUrl(delta.sourceUrl);
        applySourceKind(delta.sourceKind);
        applyDisplayedLocation(delta.displayedLocation);
        applyContainerNavigationUrl(delta.containerNavigationUrl);
        applyError(delta.errorString, delta.loadFailure);
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
        const kiriview::ImageOpenResolvedStateDelta &delta) const
    {
        return delta.clearLoadingContainerNavigationUrl && !delta.displayedLocation.has_value();
    }

    void applyTrackedLoadCompletion(const kiriview::ImageOpenResolvedStateDelta &delta)
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

    void applySourceKind(const std::optional<kiriview::ImageDocumentPageKind> &sourceKind)
    {
        if (sourceKind.has_value()) {
            m_state.setSourceKind(*sourceKind);
        }
    }

    void applyDisplayedLocation(const std::optional<kiriview::DisplayedImageLocation> &location)
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

    void applyStatus(const std::optional<kiriview::ImageDocumentStatus> &status)
    {
        if (status.has_value()) {
            m_state.setStatus(*status);
        }
    }

    void applyError(const std::optional<QString> &errorString,
        const std::optional<kiriview::ImageLoadFailure> &loadFailure)
    {
        if (loadFailure.has_value()) {
            m_state.setLoadFailure(*loadFailure);
            return;
        }

        if (errorString.has_value()) {
            m_state.setErrorString(*errorString);
        }
    }

    void applyEmbeddedMetadata(const std::optional<kiriview::EmbeddedMetadata> &metadata)
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

    kiriview::ImageDocumentState &m_state;
    kiriview::ImageDocumentState::ChangeBatch m_batch;
    kiriview::ImageDocumentRuntimePlan m_runtimePlan;
};
}

namespace kiriview {
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
    const ImageLoadSession &session, const QUrl &displayedUrl, ImageLoadFailure failure)
{
    ImageOpenTransitionContext context;
    context.session = &session;
    context.containerUrl = session.containerNavigationUrl();
    context.displayedUrl = displayedUrl;
    context.errorString = failure.userMessage;
    context.loadFailure = std::move(failure);
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
