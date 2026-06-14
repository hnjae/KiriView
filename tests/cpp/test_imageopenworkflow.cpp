// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imageopenworkflow.h"

#include "document/imagedocumentstate.h"
#include "document/imageopentransitionapplier.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace {
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::localUrl;

kiriview::ImageLoadSession loadSession(const QUrl &sourceUrl, const QUrl &imageUrl,
    const kiriview::OpenedCollectionScopeLocation &archiveCollection
    = kiriview::OpenedCollectionScopeLocation::none(),
    const QUrl &containerNavigationUrl = QUrl())
{
    return kiriview::ImageLoadSession(1,
        kiriview::ImageLoadRequest::fromLocation(
            sourceUrl, archiveCollection, containerNavigationUrl),
        kiriview::DisplayedImageLocation::fromUrl(imageUrl, archiveCollection));
}

template <typename Operation>
const Operation *findOperation(const kiriview::ImageDocumentRuntimePlan &plan)
{
    for (const kiriview::ImageDocumentRuntimeOperation &operation : plan) {
        if (const auto *payload = std::get_if<Operation>(&operation)) {
            return payload;
        }
    }

    return nullptr;
}

template <typename Operation> bool hasOperation(const kiriview::ImageDocumentRuntimePlan &plan)
{
    return findOperation<Operation>(plan) != nullptr;
}

template <typename Operation>
bool operationAtType(const kiriview::ImageDocumentRuntimePlan &plan, std::size_t index)
{
    if (index >= plan.size()) {
        return false;
    }

    return std::holds_alternative<Operation>(plan.at(index));
}

bool transitionHasEffect(
    const kiriview::ImageOpenTransition &transition, kiriview::ImageOpenEffect effect)
{
    return std::find(transition.effects.cbegin(), transition.effects.cend(), effect)
        != transition.effects.cend();
}

kiriview::ImageDocumentRuntimePlan beginSourceLoad(
    kiriview::ImageDocumentState &state, bool hasImage)
{
    return kiriview::applyImageOpenApplicationPlan(state,
        kiriview::ImageOpenWorkflow::beginSourceLoadPlan(
            kiriview::ImageOpenBeginSourceLoadSnapshot {
                hasImage,
                !state.loadingContainerNavigationUrl().isEmpty(),
            }));
}

kiriview::ImageDocumentRuntimePlan finishEmptySourceLoad(kiriview::ImageDocumentState &state)
{
    return kiriview::applyImageOpenApplicationPlan(
        state, kiriview::ImageOpenWorkflow::finishEmptySourceLoadPlan());
}

kiriview::ImageDocumentRuntimePlan resolveSourceImage(
    kiriview::ImageDocumentState &state, const kiriview::ImageLoadSession &session)
{
    return kiriview::applyImageOpenApplicationPlan(
        state, kiriview::ImageOpenWorkflow::resolveSourceImagePlan(session));
}

kiriview::ImageDocumentRuntimePlan finishUnsupportedOpenedCollectionVideoLoad(
    kiriview::ImageDocumentState &state, const kiriview::ImageLoadSession &session)
{
    return kiriview::applyImageOpenApplicationPlan(state,
        kiriview::ImageOpenWorkflow::finishUnsupportedOpenedCollectionVideoLoadPlan(session));
}

kiriview::ImageDocumentRuntimePlan finishSuccessfulImageLoad(
    kiriview::ImageDocumentState &state, const kiriview::ImageLoadSession &session)
{
    return kiriview::applyImageOpenApplicationPlan(state,
        kiriview::ImageOpenWorkflow::finishSuccessfulImageLoadPlan(
            kiriview::ImageOpenSuccessfulImageLoadSnapshot {
                session.hasContainerNavigationTarget(),
            },
            session));
}

kiriview::ImageDocumentRuntimePlan finishLoadWithError(kiriview::ImageDocumentState &state,
    const kiriview::ImageLoadSession &session, bool hasImage, const QString &errorString)
{
    const QUrl displayedUrl = state.displayedUrl();
    kiriview::ImageLoadFailure failure {
        session.imageUrl(),
        session.id(),
        kiriview::ImageLoadFailureKind::DataLoad,
        errorString,
        errorString,
        kiriview::ImageLoadFailureSeverity::Error,
        false,
    };
    return kiriview::applyImageOpenApplicationPlan(state,
        kiriview::ImageOpenWorkflow::finishLoadWithErrorPlan(
            kiriview::ImageOpenLoadErrorSnapshot {
                session.hasContainerNavigationTarget(),
                hasImage,
                !displayedUrl.isEmpty(),
            },
            session, displayedUrl, std::move(failure)));
}

kiriview::ImageDocumentRuntimePlan finishContainerNavigationLoadWithError(
    kiriview::ImageDocumentState &state, const QUrl &containerUrl, const QString &errorString)
{
    return kiriview::applyImageOpenApplicationPlan(state,
        kiriview::ImageOpenWorkflow::finishContainerNavigationLoadWithErrorPlan(
            containerUrl, errorString));
}

kiriview::ImageDocumentRuntimePlan finishAnimationLoadWithError(
    kiriview::ImageDocumentState &state, const QString &errorString)
{
    return kiriview::applyImageOpenApplicationPlan(
        state, kiriview::ImageOpenWorkflow::finishAnimationLoadWithErrorPlan(errorString));
}

}

class TestImageOpenWorkflow : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void transitionsUseExplicitSnapshotInputs();
    void sourceResolutionUsesCanonicalSessionImageUrl();
    void sourceResolutionTracksSessionSourceKind();
    void unsupportedOpenedCollectionVideoTransitionPublishesReadyVideoState();
    void firstImageLoadSuccessTransitionsToReady();
    void directArchiveImageLoadSuccessDisablesContainerNavigation();
    void replacementLoadFailureSelectsTargetError();
    void emptyContainerFailureSelectsFailedContainer();
    void animationFailureClearsImageAndResetsZoom();
    void routedLoadFailureAppliesErrorTransitions();
    void trackedLoadCompletionsClearLoadingContainerNavigationUrl();
    void workflowTransitionsClearUnsupportedOpenedCollectionVideo();
    void workflowTransitionsClearEmbeddedMetadata();
    void workflowTransitionsClearOpenStartErrorString();
    void stateChangesFollowWorkflowDeltaOrder();
};

void TestImageOpenWorkflow::transitionsUseExplicitSnapshotInputs()
{
    const kiriview::ImageOpenTransition initialLoad
        = kiriview::ImageOpenWorkflow::beginSourceLoadTransition(
            kiriview::ImageOpenBeginSourceLoadSnapshot { false, false });
    QCOMPARE(initialLoad.stateDelta.containerNavigationUrl, kiriview::ImageOpenUrlTarget::Empty);
    QCOMPARE(initialLoad.stateDelta.loading, kiriview::ImageOpenBoolTarget::True);
    QCOMPARE(initialLoad.stateDelta.status, kiriview::ImageOpenStatusTarget::Loading);
    QVERIFY(transitionHasEffect(initialLoad, kiriview::ImageOpenEffect::ClearImage));
    QVERIFY(transitionHasEffect(initialLoad, kiriview::ImageOpenEffect::ResetZoom));

    const kiriview::ImageOpenTransition routedLoad
        = kiriview::ImageOpenWorkflow::beginSourceLoadTransition(
            kiriview::ImageOpenBeginSourceLoadSnapshot { false, true });
    QCOMPARE(routedLoad.stateDelta.containerNavigationUrl, kiriview::ImageOpenUrlTarget::Unchanged);

    const kiriview::ImageOpenTransition replacementLoad
        = kiriview::ImageOpenWorkflow::beginSourceLoadTransition(
            kiriview::ImageOpenBeginSourceLoadSnapshot { true, true });
    QCOMPARE(replacementLoad.stateDelta.loading, kiriview::ImageOpenBoolTarget::True);
    QCOMPARE(replacementLoad.stateDelta.status, kiriview::ImageOpenStatusTarget::Loading);

    const kiriview::ImageOpenTransition replacementFailure
        = kiriview::ImageOpenWorkflow::finishLoadWithErrorTransition(
            kiriview::ImageOpenLoadErrorSnapshot { false, true, false });
    QCOMPARE(replacementFailure.stateDelta.sourceUrl, kiriview::ImageOpenUrlTarget::Unchanged);
    QCOMPARE(replacementFailure.stateDelta.status, kiriview::ImageOpenStatusTarget::Error);
    QVERIFY(
        !transitionHasEffect(replacementFailure, kiriview::ImageOpenEffect::UpdatePageNavigation));
}

void TestImageOpenWorkflow::sourceResolutionUsesCanonicalSessionImageUrl()
{
    kiriview::ImageDocumentState state;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl imageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    state.setSourceUrl(archiveUrl);
    state.setLoading(true);
    state.setStatus(kiriview::ImageDocumentStatus::Loading);

    const kiriview::ImageDocumentRuntimePlan plan
        = resolveSourceImage(state, loadSession(archiveUrl, imageUrl, *archiveCollection));

    QVERIFY(plan.empty());
    QCOMPARE(state.sourceUrl(), imageUrl);
    QVERIFY(state.displayedUrl().isEmpty());
    QVERIFY(state.loading());
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Loading);
}

void TestImageOpenWorkflow::sourceResolutionTracksSessionSourceKind()
{
    kiriview::ImageDocumentState state;
    const QUrl videoUrl = localUrl(QStringLiteral("/videos/clip.mp4"));
    state.setSourceUrl(videoUrl);
    state.setLoading(true);
    state.setStatus(kiriview::ImageDocumentStatus::Loading);

    const kiriview::ImageLoadSession session(1,
        kiriview::ImageLoadRequest::fromTarget(
            kiriview::ImageDocumentPageTarget { videoUrl, kiriview::ImageDocumentPageKind::Video },
            kiriview::OpenedCollectionScopeLocation::none()),
        kiriview::DisplayedImageLocation::fromUrl(videoUrl));

    const kiriview::ImageDocumentRuntimePlan plan = resolveSourceImage(state, session);

    QVERIFY(plan.empty());
    QCOMPARE(state.sourceUrl(), videoUrl);
    QCOMPARE(state.sourceKind(), kiriview::ImageDocumentPageKind::Video);
    QVERIFY(state.loading());
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Loading);
}

void TestImageOpenWorkflow::unsupportedOpenedCollectionVideoTransitionPublishesReadyVideoState()
{
    kiriview::ImageDocumentState state;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl videoUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.mp4"));
    const kiriview::DisplayedImageLocation location
        = kiriview::DisplayedImageLocation::fromUrl(videoUrl, *archiveCollection);
    kiriview::EmbeddedMetadata metadata;
    metadata.cameraMake = QStringLiteral("Kiri Camera");
    state.setLoading(true);
    state.setLoadingContainerNavigationUrl(archiveUrl);
    state.setErrorString(QStringLiteral("previous error"));
    state.setEmbeddedMetadata(metadata);

    const kiriview::ImageLoadSession session(8,
        kiriview::ImageLoadRequest::fromTarget(
            kiriview::ImageDocumentPageTarget { videoUrl, kiriview::ImageDocumentPageKind::Video },
            *archiveCollection),
        location);

    const kiriview::ImageDocumentRuntimePlan plan
        = finishUnsupportedOpenedCollectionVideoLoad(state, session);

    QCOMPARE(state.sourceUrl(), videoUrl);
    QCOMPARE(state.sourceKind(), kiriview::ImageDocumentPageKind::Video);
    QCOMPARE(state.displayedImageLocation(), location);
    QCOMPARE(state.containerNavigationUrl(), kiriview::containerNavigationUrlForLocation(location));
    QVERIFY(state.errorString().isEmpty());
    QVERIFY(state.embeddedMetadata().isEmpty());
    QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Ready);
    QVERIFY(state.unsupportedOpenedCollectionVideo());
    QCOMPARE(plan.size(), std::size_t(3));
    QVERIFY(operationAtType<kiriview::FinishSpreadTransitionOperation>(plan, 0));
    QVERIFY(operationAtType<kiriview::ClearSecondaryPageOperation>(plan, 1));
    QVERIFY(operationAtType<kiriview::UpdatePageNavigationOperation>(plan, 2));
}

void TestImageOpenWorkflow::firstImageLoadSuccessTransitionsToReady()
{
    kiriview::ImageDocumentState state;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    state.setSourceUrl(imageUrl);

    const kiriview::ImageDocumentRuntimePlan beginPlan = beginSourceLoad(state, false);
    QVERIFY(hasOperation<kiriview::ClearPresentationImageOperation>(beginPlan));
    QVERIFY(hasOperation<kiriview::ResetZoomOperation>(beginPlan));
    QCOMPARE(beginPlan.size(), std::size_t(10));
    QVERIFY(operationAtType<kiriview::ClearMediaEntrySourceOperation>(beginPlan, 0));
    QVERIFY(operationAtType<kiriview::ClearPresentationImageOperation>(beginPlan, 6));
    QVERIFY(operationAtType<kiriview::ResetZoomOperation>(beginPlan, 9));
    QVERIFY(state.loading());
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Loading);

    const kiriview::ImageDocumentRuntimePlan successPlan
        = finishSuccessfulImageLoad(state, loadSession(imageUrl, imageUrl));
    QVERIFY(hasOperation<kiriview::UpdatePageNavigationOperation>(successPlan));
    QCOMPARE(successPlan.size(), std::size_t(2));
    QVERIFY(operationAtType<kiriview::UpdatePageNavigationOperation>(successPlan, 0));
    QVERIFY(operationAtType<kiriview::ScheduleAdjacentImagePredecodeOperation>(successPlan, 1));
    QCOMPARE(state.sourceUrl(), imageUrl);
    QCOMPARE(state.displayedUrl(), imageUrl);
    QVERIFY(state.containerNavigationUrl().isEmpty());
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Ready);
    QVERIFY(state.errorString().isEmpty());
}

void TestImageOpenWorkflow::directArchiveImageLoadSuccessDisablesContainerNavigation()
{
    kiriview::ImageDocumentState state;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl imageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));

    state.setSourceUrl(archiveUrl);

    const kiriview::ImageDocumentRuntimePlan successPlan
        = finishSuccessfulImageLoad(state, loadSession(archiveUrl, imageUrl, *archiveCollection));

    QVERIFY(hasOperation<kiriview::UpdatePageNavigationOperation>(successPlan));
    QCOMPARE(state.sourceUrl(), imageUrl);
    QCOMPARE(state.displayedUrl(), imageUrl);
    QCOMPARE(state.windowTitleFileName(), QStringLiteral("book.zip"));
    QVERIFY(state.containerNavigationUrl().isEmpty());
    QVERIFY(!state.containerNavigationAvailable());
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Ready);
}

void TestImageOpenWorkflow::replacementLoadFailureSelectsTargetError()
{
    kiriview::ImageDocumentState state;
    const QUrl replacementUrl = localUrl(QStringLiteral("/images/missing.png"));
    state.setSourceUrl(replacementUrl);
    state.setLoading(true);
    state.setStatus(kiriview::ImageDocumentStatus::Loading);

    const kiriview::ImageDocumentRuntimePlan plan = finishLoadWithError(
        state, loadSession(replacementUrl, replacementUrl), true, QStringLiteral("missing"));
    QVERIFY(!hasOperation<kiriview::ClearPresentationImageOperation>(plan));
    QVERIFY(!hasOperation<kiriview::UpdatePageNavigationOperation>(plan));
    QVERIFY(!hasOperation<kiriview::ScheduleAdjacentImagePredecodeOperation>(plan));
    QCOMPARE(state.sourceUrl(), replacementUrl);
    QCOMPARE(state.displayedUrl(), QUrl());
    QCOMPARE(state.errorString(), QStringLiteral("missing"));
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Error);
}

void TestImageOpenWorkflow::emptyContainerFailureSelectsFailedContainer()
{
    kiriview::ImageDocumentState state;
    const QUrl containerUrl = localUrl(QStringLiteral("/images/empty/"));
    state.setLoading(true);
    state.setLoadingContainerNavigationUrl(containerUrl);

    const kiriview::ImageDocumentRuntimePlan plan
        = finishContainerNavigationLoadWithError(state, containerUrl, QStringLiteral("empty"));
    QVERIFY(hasOperation<kiriview::ClearPresentationImageOperation>(plan));
    const auto *prepareFailedContainer
        = findOperation<kiriview::PrepareFailedContainerOperation>(plan);
    QVERIFY(prepareFailedContainer != nullptr);
    QCOMPARE(prepareFailedContainer->containerUrl, containerUrl);
    QCOMPARE(state.sourceUrl(), containerUrl);
    QCOMPARE(state.containerNavigationUrl(), containerUrl);
    QCOMPARE(state.errorString(), QStringLiteral("empty"));
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Error);
}

void TestImageOpenWorkflow::animationFailureClearsImageAndResetsZoom()
{
    kiriview::ImageDocumentState state;
    const QUrl displayedUrl = localUrl(QStringLiteral("/images/animated.png"));
    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromUrl(displayedUrl));
    state.setContainerNavigationUrl(localUrl(QStringLiteral("/images/")));
    state.setLoading(true);
    state.setStatus(kiriview::ImageDocumentStatus::Ready);

    const kiriview::ImageDocumentRuntimePlan plan
        = finishAnimationLoadWithError(state, QStringLiteral("animation failed"));

    QVERIFY(hasOperation<kiriview::ClearPresentationImageOperation>(plan));
    QVERIFY(hasOperation<kiriview::ResetZoomOperation>(plan));
    QVERIFY(state.containerNavigationUrl().isEmpty());
    QCOMPARE(state.errorString(), QStringLiteral("animation failed"));
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Error);
}

void TestImageOpenWorkflow::routedLoadFailureAppliesErrorTransitions()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const kiriview::ImageLoadSession containerNavigationSession = loadSession(
        containerUrl, imageUrl, kiriview::OpenedCollectionScopeLocation::none(), containerUrl);
    const kiriview::ImageLoadSession imageSession = loadSession(imageUrl, imageUrl);

    {
        kiriview::ImageDocumentState state;
        state.setLoading(true);
        const kiriview::ImageDocumentRuntimePlan plan
            = finishLoadWithError(state, containerNavigationSession, true, QStringLiteral("empty"));
        QVERIFY(hasOperation<kiriview::ClearPresentationImageOperation>(plan));
        QVERIFY(hasOperation<kiriview::PrepareFailedContainerOperation>(plan));
        QCOMPARE(state.sourceUrl(), containerUrl);
        QCOMPARE(state.containerNavigationUrl(), containerUrl);
        QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Error);
    }

    {
        kiriview::ImageDocumentState state;
        state.setSourceUrl(localUrl(QStringLiteral("/images/missing.png")));
        state.setLoading(true);
        state.setStatus(kiriview::ImageDocumentStatus::Loading);
        const kiriview::ImageDocumentRuntimePlan plan
            = finishLoadWithError(state, imageSession, true, QStringLiteral("missing"));
        QVERIFY(!hasOperation<kiriview::ClearPresentationImageOperation>(plan));
        QVERIFY(!hasOperation<kiriview::UpdatePageNavigationOperation>(plan));
        QCOMPARE(state.sourceUrl(), localUrl(QStringLiteral("/images/missing.png")));
        QCOMPARE(state.displayedUrl(), QUrl());
        QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Error);
    }

    {
        kiriview::ImageDocumentState state;
        state.setLoading(true);
        const kiriview::ImageDocumentRuntimePlan plan
            = finishLoadWithError(state, imageSession, false, QStringLiteral("missing"));
        QVERIFY(!hasOperation<kiriview::ClearPresentationImageOperation>(plan));
        QCOMPARE(state.sourceUrl(), QUrl());
        QCOMPARE(state.containerNavigationUrl(), QUrl());
        QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Error);
    }
}

void TestImageOpenWorkflow::trackedLoadCompletionsClearLoadingContainerNavigationUrl()
{
    const QUrl loadingContainerUrl = localUrl(QStringLiteral("/books/loading.cbz"));
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));

    {
        kiriview::ImageDocumentState state;
        state.setLoading(true);
        state.setLoadingContainerNavigationUrl(loadingContainerUrl);

        finishEmptySourceLoad(state);

        QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    }

    {
        kiriview::ImageDocumentState state;
        state.setLoading(true);
        state.setLoadingContainerNavigationUrl(loadingContainerUrl);

        finishSuccessfulImageLoad(state, loadSession(imageUrl, imageUrl));

        QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    }

    {
        kiriview::ImageDocumentState state;
        state.setLoading(true);
        state.setLoadingContainerNavigationUrl(loadingContainerUrl);

        finishLoadWithError(
            state, loadSession(imageUrl, imageUrl), true, QStringLiteral("missing"));

        QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    }

    {
        kiriview::ImageDocumentState state;
        state.setLoading(true);
        state.setLoadingContainerNavigationUrl(loadingContainerUrl);

        finishLoadWithError(
            state, loadSession(imageUrl, imageUrl), false, QStringLiteral("missing"));

        QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    }

    {
        kiriview::ImageDocumentState state;
        state.setLoading(true);
        state.setLoadingContainerNavigationUrl(loadingContainerUrl);

        finishContainerNavigationLoadWithError(
            state, loadingContainerUrl, QStringLiteral("missing"));

        QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    }

    {
        kiriview::ImageDocumentState state;
        state.setLoading(true);
        state.setLoadingContainerNavigationUrl(loadingContainerUrl);

        finishAnimationLoadWithError(state, QStringLiteral("animation failed"));

        QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    }
}

void TestImageOpenWorkflow::workflowTransitionsClearUnsupportedOpenedCollectionVideo()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));

    {
        kiriview::ImageDocumentState state;
        state.setUnsupportedOpenedCollectionVideo(true);

        beginSourceLoad(state, false);

        QVERIFY(!state.unsupportedOpenedCollectionVideo());
    }

    {
        kiriview::ImageDocumentState state;
        state.setUnsupportedOpenedCollectionVideo(true);

        finishEmptySourceLoad(state);

        QVERIFY(!state.unsupportedOpenedCollectionVideo());
    }

    {
        kiriview::ImageDocumentState state;
        state.setUnsupportedOpenedCollectionVideo(true);

        finishSuccessfulImageLoad(state, loadSession(imageUrl, imageUrl));

        QVERIFY(!state.unsupportedOpenedCollectionVideo());
    }

    {
        kiriview::ImageDocumentState state;
        state.setUnsupportedOpenedCollectionVideo(true);
        state.setSourceUrl(localUrl(QStringLiteral("/images/missing.png")));
        state.setLoading(true);
        state.setStatus(kiriview::ImageDocumentStatus::Loading);

        finishLoadWithError(
            state, loadSession(imageUrl, imageUrl), true, QStringLiteral("missing"));

        QVERIFY(!state.unsupportedOpenedCollectionVideo());
    }

    {
        kiriview::ImageDocumentState state;
        state.setUnsupportedOpenedCollectionVideo(true);
        state.setLoading(true);
        state.setLoadingContainerNavigationUrl(containerUrl);

        finishContainerNavigationLoadWithError(state, containerUrl, QStringLiteral("empty"));

        QVERIFY(!state.unsupportedOpenedCollectionVideo());
    }
}

void TestImageOpenWorkflow::workflowTransitionsClearEmbeddedMetadata()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    auto publishMetadata = [](kiriview::ImageDocumentState &state) {
        kiriview::EmbeddedMetadata metadata;
        metadata.cameraMake = QStringLiteral("Kiri Camera");
        state.setEmbeddedMetadata(metadata);
    };

    {
        kiriview::ImageDocumentState state;
        publishMetadata(state);

        beginSourceLoad(state, false);

        QVERIFY(state.embeddedMetadata().isEmpty());
    }

    {
        kiriview::ImageDocumentState state;
        publishMetadata(state);

        finishEmptySourceLoad(state);

        QVERIFY(state.embeddedMetadata().isEmpty());
    }

    {
        kiriview::ImageDocumentState state;
        publishMetadata(state);
        state.setSourceUrl(localUrl(QStringLiteral("/images/missing.png")));
        state.setLoading(true);
        state.setStatus(kiriview::ImageDocumentStatus::Loading);

        finishLoadWithError(
            state, loadSession(imageUrl, imageUrl), true, QStringLiteral("missing"));

        QVERIFY(state.embeddedMetadata().isEmpty());
    }
}

void TestImageOpenWorkflow::workflowTransitionsClearOpenStartErrorString()
{
    {
        kiriview::ImageDocumentState state;
        state.setErrorString(QStringLiteral("previous error"));

        beginSourceLoad(state, false);

        QVERIFY(state.errorString().isEmpty());
    }

    {
        kiriview::ImageDocumentState state;
        state.setErrorString(QStringLiteral("previous error"));

        finishEmptySourceLoad(state);

        QVERIFY(state.errorString().isEmpty());
    }
}

void TestImageOpenWorkflow::stateChangesFollowWorkflowDeltaOrder()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));

    {
        std::vector<kiriview::ImageDocumentChange> changes;
        kiriview::ImageDocumentState state(
            [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });

        beginSourceLoad(state, false);

        QCOMPARE(changes.size(), std::size_t(3));
        QCOMPARE(changes.at(0), kiriview::ImageDocumentChange::EmbeddedMetadata);
        QCOMPARE(changes.at(1), kiriview::ImageDocumentChange::Loading);
        QCOMPARE(changes.at(2), kiriview::ImageDocumentChange::Status);
    }

    {
        std::vector<kiriview::ImageDocumentChange> changes;
        kiriview::ImageDocumentState state(
            [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });

        finishSuccessfulImageLoad(state, loadSession(imageUrl, imageUrl));

        QCOMPARE(changes.size(), std::size_t(4));
        QCOMPARE(changes.at(0), kiriview::ImageDocumentChange::SourceUrl);
        QCOMPARE(changes.at(1), kiriview::ImageDocumentChange::DisplayedUrl);
        QCOMPARE(changes.at(2), kiriview::ImageDocumentChange::WindowTitleFileName);
        QCOMPARE(changes.at(3), kiriview::ImageDocumentChange::Status);
    }

    {
        std::vector<kiriview::ImageDocumentChange> changes;
        kiriview::ImageDocumentState state(
            [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });
        const QUrl replacementUrl = localUrl(QStringLiteral("/images/missing.png"));
        state.setSourceUrl(replacementUrl);
        state.setLoading(true);
        state.setStatus(kiriview::ImageDocumentStatus::Loading);
        changes.clear();

        finishLoadWithError(
            state, loadSession(replacementUrl, replacementUrl), true, QStringLiteral("missing"));

        QCOMPARE(changes.size(), std::size_t(4));
        QCOMPARE(changes.at(0), kiriview::ImageDocumentChange::Loading);
        QCOMPARE(changes.at(1), kiriview::ImageDocumentChange::ErrorString);
        QCOMPARE(changes.at(2), kiriview::ImageDocumentChange::EmbeddedMetadata);
        QCOMPARE(changes.at(3), kiriview::ImageDocumentChange::Status);
    }
}

QTEST_GUILESS_MAIN(TestImageOpenWorkflow)

#include "test_imageopenworkflow.moc"
