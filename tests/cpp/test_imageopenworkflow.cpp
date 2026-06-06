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
#include <variant>
#include <vector>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::localUrl;

KiriView::ImageLoadSession loadSession(const QUrl &sourceUrl, const QUrl &imageUrl,
    const KiriView::OpenedCollectionScopeLocation &archiveCollection
    = KiriView::OpenedCollectionScopeLocation::none(),
    const QUrl &containerNavigationUrl = QUrl())
{
    return KiriView::ImageLoadSession(1,
        KiriView::ImageLoadRequest::fromLocation(
            sourceUrl, archiveCollection, containerNavigationUrl),
        KiriView::DisplayedImageLocation::fromUrl(imageUrl, archiveCollection));
}

template <typename Operation>
const Operation *findOperation(const KiriView::ImageDocumentRuntimePlan &plan)
{
    for (const KiriView::ImageDocumentRuntimeOperation &operation : plan) {
        if (const auto *payload = std::get_if<Operation>(&operation)) {
            return payload;
        }
    }

    return nullptr;
}

template <typename Operation> bool hasOperation(const KiriView::ImageDocumentRuntimePlan &plan)
{
    return findOperation<Operation>(plan) != nullptr;
}

template <typename Operation>
bool operationAtType(const KiriView::ImageDocumentRuntimePlan &plan, std::size_t index)
{
    if (index >= plan.size()) {
        return false;
    }

    return std::holds_alternative<Operation>(plan.at(index));
}

bool transitionHasEffect(
    const KiriView::ImageOpenTransition &transition, KiriView::ImageOpenEffect effect)
{
    return std::find(transition.effects.cbegin(), transition.effects.cend(), effect)
        != transition.effects.cend();
}

KiriView::ImageDocumentRuntimePlan beginSourceLoad(
    KiriView::ImageDocumentState &state, bool hasImage)
{
    return KiriView::applyImageOpenApplicationPlan(state,
        KiriView::ImageOpenWorkflow::beginSourceLoadPlan(
            KiriView::ImageOpenBeginSourceLoadSnapshot {
                hasImage,
                !state.loadingContainerNavigationUrl().isEmpty(),
            }));
}

KiriView::ImageDocumentRuntimePlan finishEmptySourceLoad(KiriView::ImageDocumentState &state)
{
    return KiriView::applyImageOpenApplicationPlan(
        state, KiriView::ImageOpenWorkflow::finishEmptySourceLoadPlan());
}

KiriView::ImageDocumentRuntimePlan resolveSourceImage(
    KiriView::ImageDocumentState &state, const KiriView::ImageLoadSession &session)
{
    return KiriView::applyImageOpenApplicationPlan(
        state, KiriView::ImageOpenWorkflow::resolveSourceImagePlan(session));
}

KiriView::ImageDocumentRuntimePlan finishUnsupportedOpenedCollectionVideoLoad(
    KiriView::ImageDocumentState &state, const KiriView::ImageLoadSession &session)
{
    return KiriView::applyImageOpenApplicationPlan(state,
        KiriView::ImageOpenWorkflow::finishUnsupportedOpenedCollectionVideoLoadPlan(session));
}

KiriView::ImageDocumentRuntimePlan finishSuccessfulImageLoad(
    KiriView::ImageDocumentState &state, const KiriView::ImageLoadSession &session)
{
    return KiriView::applyImageOpenApplicationPlan(state,
        KiriView::ImageOpenWorkflow::finishSuccessfulImageLoadPlan(
            KiriView::ImageOpenSuccessfulImageLoadSnapshot {
                session.hasContainerNavigationTarget(),
            },
            session));
}

KiriView::ImageDocumentRuntimePlan finishLoadWithError(KiriView::ImageDocumentState &state,
    const KiriView::ImageLoadSession &session, bool hasImage, const QString &errorString)
{
    const QUrl displayedUrl = state.displayedUrl();
    return KiriView::applyImageOpenApplicationPlan(state,
        KiriView::ImageOpenWorkflow::finishLoadWithErrorPlan(
            KiriView::ImageOpenLoadErrorSnapshot {
                session.hasContainerNavigationTarget(),
                hasImage,
                !displayedUrl.isEmpty(),
            },
            session, displayedUrl, errorString));
}

KiriView::ImageDocumentRuntimePlan finishContainerNavigationLoadWithError(
    KiriView::ImageDocumentState &state, const QUrl &containerUrl, const QString &errorString)
{
    return KiriView::applyImageOpenApplicationPlan(state,
        KiriView::ImageOpenWorkflow::finishContainerNavigationLoadWithErrorPlan(
            containerUrl, errorString));
}

KiriView::ImageDocumentRuntimePlan finishAnimationLoadWithError(
    KiriView::ImageDocumentState &state, const QString &errorString)
{
    return KiriView::applyImageOpenApplicationPlan(
        state, KiriView::ImageOpenWorkflow::finishAnimationLoadWithErrorPlan(errorString));
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
    void stateChangesFollowWorkflowDeltaOrder();
};

void TestImageOpenWorkflow::transitionsUseExplicitSnapshotInputs()
{
    const KiriView::ImageOpenTransition initialLoad
        = KiriView::ImageOpenWorkflow::beginSourceLoadTransition(
            KiriView::ImageOpenBeginSourceLoadSnapshot { false, false });
    QCOMPARE(initialLoad.stateDelta.containerNavigationUrl, KiriView::ImageOpenUrlTarget::Empty);
    QCOMPARE(initialLoad.stateDelta.loading, KiriView::ImageOpenBoolTarget::True);
    QCOMPARE(initialLoad.stateDelta.status, KiriView::ImageOpenStatusTarget::Loading);
    QVERIFY(transitionHasEffect(initialLoad, KiriView::ImageOpenEffect::ClearImage));
    QVERIFY(transitionHasEffect(initialLoad, KiriView::ImageOpenEffect::ResetZoom));

    const KiriView::ImageOpenTransition routedLoad
        = KiriView::ImageOpenWorkflow::beginSourceLoadTransition(
            KiriView::ImageOpenBeginSourceLoadSnapshot { false, true });
    QCOMPARE(routedLoad.stateDelta.containerNavigationUrl, KiriView::ImageOpenUrlTarget::Unchanged);

    const KiriView::ImageOpenTransition replacementLoad
        = KiriView::ImageOpenWorkflow::beginSourceLoadTransition(
            KiriView::ImageOpenBeginSourceLoadSnapshot { true, true });
    QCOMPARE(replacementLoad.stateDelta.loading, KiriView::ImageOpenBoolTarget::True);
    QCOMPARE(replacementLoad.stateDelta.status, KiriView::ImageOpenStatusTarget::Loading);

    const KiriView::ImageOpenTransition replacementFailure
        = KiriView::ImageOpenWorkflow::finishLoadWithErrorTransition(
            KiriView::ImageOpenLoadErrorSnapshot { false, true, false });
    QCOMPARE(replacementFailure.stateDelta.sourceUrl, KiriView::ImageOpenUrlTarget::Unchanged);
    QCOMPARE(replacementFailure.stateDelta.status, KiriView::ImageOpenStatusTarget::Error);
    QVERIFY(
        !transitionHasEffect(replacementFailure, KiriView::ImageOpenEffect::UpdatePageNavigation));
}

void TestImageOpenWorkflow::sourceResolutionUsesCanonicalSessionImageUrl()
{
    KiriView::ImageDocumentState state;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl imageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    state.setSourceUrl(archiveUrl);
    state.setLoading(true);
    state.setStatus(KiriView::ImageDocumentStatus::Loading);

    const KiriView::ImageDocumentRuntimePlan plan
        = resolveSourceImage(state, loadSession(archiveUrl, imageUrl, *archiveCollection));

    QVERIFY(plan.empty());
    QCOMPARE(state.sourceUrl(), imageUrl);
    QVERIFY(state.displayedUrl().isEmpty());
    QVERIFY(state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Loading);
}

void TestImageOpenWorkflow::sourceResolutionTracksSessionSourceKind()
{
    KiriView::ImageDocumentState state;
    const QUrl videoUrl = localUrl(QStringLiteral("/videos/clip.mp4"));
    state.setSourceUrl(videoUrl);
    state.setLoading(true);
    state.setStatus(KiriView::ImageDocumentStatus::Loading);

    const KiriView::ImageLoadSession session(1,
        KiriView::ImageLoadRequest::fromTarget(
            KiriView::ImageDocumentPageTarget { videoUrl, KiriView::ImageDocumentPageKind::Video },
            KiriView::OpenedCollectionScopeLocation::none()),
        KiriView::DisplayedImageLocation::fromUrl(videoUrl));

    const KiriView::ImageDocumentRuntimePlan plan = resolveSourceImage(state, session);

    QVERIFY(plan.empty());
    QCOMPARE(state.sourceUrl(), videoUrl);
    QCOMPARE(state.sourceKind(), KiriView::ImageDocumentPageKind::Video);
    QVERIFY(state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Loading);
}

void TestImageOpenWorkflow::unsupportedOpenedCollectionVideoTransitionPublishesReadyVideoState()
{
    KiriView::ImageDocumentState state;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl videoUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.mp4"));
    const KiriView::DisplayedImageLocation location
        = KiriView::DisplayedImageLocation::fromUrl(videoUrl, *archiveCollection);
    KiriView::EmbeddedMetadata metadata;
    metadata.cameraMake = QStringLiteral("Kiri Camera");
    state.setLoading(true);
    state.setLoadingContainerNavigationUrl(archiveUrl);
    state.setErrorString(QStringLiteral("previous error"));
    state.setEmbeddedMetadata(metadata);

    const KiriView::ImageLoadSession session(8,
        KiriView::ImageLoadRequest::fromTarget(
            KiriView::ImageDocumentPageTarget { videoUrl, KiriView::ImageDocumentPageKind::Video },
            *archiveCollection),
        location);

    const KiriView::ImageDocumentRuntimePlan plan
        = finishUnsupportedOpenedCollectionVideoLoad(state, session);

    QCOMPARE(state.sourceUrl(), videoUrl);
    QCOMPARE(state.sourceKind(), KiriView::ImageDocumentPageKind::Video);
    QCOMPARE(state.displayedImageLocation(), location);
    QCOMPARE(state.containerNavigationUrl(), KiriView::containerNavigationUrlForLocation(location));
    QVERIFY(state.errorString().isEmpty());
    QVERIFY(state.embeddedMetadata().isEmpty());
    QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Ready);
    QVERIFY(state.unsupportedOpenedCollectionVideo());
    QCOMPARE(plan.size(), std::size_t(3));
    QVERIFY(operationAtType<KiriView::FinishSpreadTransitionOperation>(plan, 0));
    QVERIFY(operationAtType<KiriView::ClearSecondaryPageOperation>(plan, 1));
    QVERIFY(operationAtType<KiriView::UpdatePageNavigationOperation>(plan, 2));
}

void TestImageOpenWorkflow::firstImageLoadSuccessTransitionsToReady()
{
    KiriView::ImageDocumentState state;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    state.setSourceUrl(imageUrl);

    const KiriView::ImageDocumentRuntimePlan beginPlan = beginSourceLoad(state, false);
    QVERIFY(hasOperation<KiriView::ClearPresentationImageOperation>(beginPlan));
    QVERIFY(hasOperation<KiriView::ResetZoomOperation>(beginPlan));
    QCOMPARE(beginPlan.size(), std::size_t(10));
    QVERIFY(operationAtType<KiriView::ClearMediaEntrySourceOperation>(beginPlan, 0));
    QVERIFY(operationAtType<KiriView::ClearPresentationImageOperation>(beginPlan, 6));
    QVERIFY(operationAtType<KiriView::ResetZoomOperation>(beginPlan, 9));
    QVERIFY(state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Loading);

    const KiriView::ImageDocumentRuntimePlan successPlan
        = finishSuccessfulImageLoad(state, loadSession(imageUrl, imageUrl));
    QVERIFY(hasOperation<KiriView::UpdatePageNavigationOperation>(successPlan));
    QCOMPARE(successPlan.size(), std::size_t(2));
    QVERIFY(operationAtType<KiriView::UpdatePageNavigationOperation>(successPlan, 0));
    QVERIFY(operationAtType<KiriView::ScheduleAdjacentImagePredecodeOperation>(successPlan, 1));
    QCOMPARE(state.sourceUrl(), imageUrl);
    QCOMPARE(state.displayedUrl(), imageUrl);
    QVERIFY(state.containerNavigationUrl().isEmpty());
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Ready);
    QVERIFY(state.errorString().isEmpty());
}

void TestImageOpenWorkflow::directArchiveImageLoadSuccessDisablesContainerNavigation()
{
    KiriView::ImageDocumentState state;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl imageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));

    state.setSourceUrl(archiveUrl);

    const KiriView::ImageDocumentRuntimePlan successPlan
        = finishSuccessfulImageLoad(state, loadSession(archiveUrl, imageUrl, *archiveCollection));

    QVERIFY(hasOperation<KiriView::UpdatePageNavigationOperation>(successPlan));
    QCOMPARE(state.sourceUrl(), imageUrl);
    QCOMPARE(state.displayedUrl(), imageUrl);
    QCOMPARE(state.windowTitleFileName(), QStringLiteral("book.zip"));
    QVERIFY(state.containerNavigationUrl().isEmpty());
    QVERIFY(!state.containerNavigationAvailable());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Ready);
}

void TestImageOpenWorkflow::replacementLoadFailureSelectsTargetError()
{
    KiriView::ImageDocumentState state;
    const QUrl replacementUrl = localUrl(QStringLiteral("/images/missing.png"));
    state.setSourceUrl(replacementUrl);
    state.setLoading(true);
    state.setStatus(KiriView::ImageDocumentStatus::Loading);

    const KiriView::ImageDocumentRuntimePlan plan = finishLoadWithError(
        state, loadSession(replacementUrl, replacementUrl), true, QStringLiteral("missing"));
    QVERIFY(!hasOperation<KiriView::ClearPresentationImageOperation>(plan));
    QVERIFY(!hasOperation<KiriView::UpdatePageNavigationOperation>(plan));
    QVERIFY(!hasOperation<KiriView::ScheduleAdjacentImagePredecodeOperation>(plan));
    QCOMPARE(state.sourceUrl(), replacementUrl);
    QCOMPARE(state.displayedUrl(), QUrl());
    QCOMPARE(state.errorString(), QStringLiteral("missing"));
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Error);
}

void TestImageOpenWorkflow::emptyContainerFailureSelectsFailedContainer()
{
    KiriView::ImageDocumentState state;
    const QUrl containerUrl = localUrl(QStringLiteral("/images/empty/"));
    state.setLoading(true);
    state.setLoadingContainerNavigationUrl(containerUrl);

    const KiriView::ImageDocumentRuntimePlan plan
        = finishContainerNavigationLoadWithError(state, containerUrl, QStringLiteral("empty"));
    QVERIFY(hasOperation<KiriView::ClearPresentationImageOperation>(plan));
    const auto *prepareFailedContainer
        = findOperation<KiriView::PrepareFailedContainerOperation>(plan);
    QVERIFY(prepareFailedContainer != nullptr);
    QCOMPARE(prepareFailedContainer->containerUrl, containerUrl);
    QCOMPARE(state.sourceUrl(), containerUrl);
    QCOMPARE(state.containerNavigationUrl(), containerUrl);
    QCOMPARE(state.errorString(), QStringLiteral("empty"));
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Error);
}

void TestImageOpenWorkflow::animationFailureClearsImageAndResetsZoom()
{
    KiriView::ImageDocumentState state;
    const QUrl displayedUrl = localUrl(QStringLiteral("/images/animated.png"));
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(displayedUrl));
    state.setContainerNavigationUrl(localUrl(QStringLiteral("/images/")));
    state.setLoading(true);
    state.setStatus(KiriView::ImageDocumentStatus::Ready);

    const KiriView::ImageDocumentRuntimePlan plan
        = finishAnimationLoadWithError(state, QStringLiteral("animation failed"));

    QVERIFY(hasOperation<KiriView::ClearPresentationImageOperation>(plan));
    QVERIFY(hasOperation<KiriView::ResetZoomOperation>(plan));
    QVERIFY(state.containerNavigationUrl().isEmpty());
    QCOMPARE(state.errorString(), QStringLiteral("animation failed"));
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Error);
}

void TestImageOpenWorkflow::routedLoadFailureAppliesErrorTransitions()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const KiriView::ImageLoadSession containerNavigationSession = loadSession(
        containerUrl, imageUrl, KiriView::OpenedCollectionScopeLocation::none(), containerUrl);
    const KiriView::ImageLoadSession imageSession = loadSession(imageUrl, imageUrl);

    {
        KiriView::ImageDocumentState state;
        state.setLoading(true);
        const KiriView::ImageDocumentRuntimePlan plan
            = finishLoadWithError(state, containerNavigationSession, true, QStringLiteral("empty"));
        QVERIFY(hasOperation<KiriView::ClearPresentationImageOperation>(plan));
        QVERIFY(hasOperation<KiriView::PrepareFailedContainerOperation>(plan));
        QCOMPARE(state.sourceUrl(), containerUrl);
        QCOMPARE(state.containerNavigationUrl(), containerUrl);
        QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Error);
    }

    {
        KiriView::ImageDocumentState state;
        state.setSourceUrl(localUrl(QStringLiteral("/images/missing.png")));
        state.setLoading(true);
        state.setStatus(KiriView::ImageDocumentStatus::Loading);
        const KiriView::ImageDocumentRuntimePlan plan
            = finishLoadWithError(state, imageSession, true, QStringLiteral("missing"));
        QVERIFY(!hasOperation<KiriView::ClearPresentationImageOperation>(plan));
        QVERIFY(!hasOperation<KiriView::UpdatePageNavigationOperation>(plan));
        QCOMPARE(state.sourceUrl(), localUrl(QStringLiteral("/images/missing.png")));
        QCOMPARE(state.displayedUrl(), QUrl());
        QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Error);
    }

    {
        KiriView::ImageDocumentState state;
        state.setLoading(true);
        const KiriView::ImageDocumentRuntimePlan plan
            = finishLoadWithError(state, imageSession, false, QStringLiteral("missing"));
        QVERIFY(!hasOperation<KiriView::ClearPresentationImageOperation>(plan));
        QCOMPARE(state.sourceUrl(), QUrl());
        QCOMPARE(state.containerNavigationUrl(), QUrl());
        QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Error);
    }
}

void TestImageOpenWorkflow::trackedLoadCompletionsClearLoadingContainerNavigationUrl()
{
    const QUrl loadingContainerUrl = localUrl(QStringLiteral("/books/loading.cbz"));
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));

    {
        KiriView::ImageDocumentState state;
        state.setLoading(true);
        state.setLoadingContainerNavigationUrl(loadingContainerUrl);

        finishEmptySourceLoad(state);

        QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    }

    {
        KiriView::ImageDocumentState state;
        state.setLoading(true);
        state.setLoadingContainerNavigationUrl(loadingContainerUrl);

        finishSuccessfulImageLoad(state, loadSession(imageUrl, imageUrl));

        QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    }

    {
        KiriView::ImageDocumentState state;
        state.setLoading(true);
        state.setLoadingContainerNavigationUrl(loadingContainerUrl);

        finishLoadWithError(
            state, loadSession(imageUrl, imageUrl), true, QStringLiteral("missing"));

        QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    }

    {
        KiriView::ImageDocumentState state;
        state.setLoading(true);
        state.setLoadingContainerNavigationUrl(loadingContainerUrl);

        finishLoadWithError(
            state, loadSession(imageUrl, imageUrl), false, QStringLiteral("missing"));

        QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    }

    {
        KiriView::ImageDocumentState state;
        state.setLoading(true);
        state.setLoadingContainerNavigationUrl(loadingContainerUrl);

        finishContainerNavigationLoadWithError(
            state, loadingContainerUrl, QStringLiteral("missing"));

        QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    }

    {
        KiriView::ImageDocumentState state;
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
        KiriView::ImageDocumentState state;
        state.setUnsupportedOpenedCollectionVideo(true);

        beginSourceLoad(state, false);

        QVERIFY(!state.unsupportedOpenedCollectionVideo());
    }

    {
        KiriView::ImageDocumentState state;
        state.setUnsupportedOpenedCollectionVideo(true);

        finishEmptySourceLoad(state);

        QVERIFY(!state.unsupportedOpenedCollectionVideo());
    }

    {
        KiriView::ImageDocumentState state;
        state.setUnsupportedOpenedCollectionVideo(true);

        finishSuccessfulImageLoad(state, loadSession(imageUrl, imageUrl));

        QVERIFY(!state.unsupportedOpenedCollectionVideo());
    }

    {
        KiriView::ImageDocumentState state;
        state.setUnsupportedOpenedCollectionVideo(true);
        state.setSourceUrl(localUrl(QStringLiteral("/images/missing.png")));
        state.setLoading(true);
        state.setStatus(KiriView::ImageDocumentStatus::Loading);

        finishLoadWithError(
            state, loadSession(imageUrl, imageUrl), true, QStringLiteral("missing"));

        QVERIFY(!state.unsupportedOpenedCollectionVideo());
    }

    {
        KiriView::ImageDocumentState state;
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
    auto publishMetadata = [](KiriView::ImageDocumentState &state) {
        KiriView::EmbeddedMetadata metadata;
        metadata.cameraMake = QStringLiteral("Kiri Camera");
        state.setEmbeddedMetadata(metadata);
    };

    {
        KiriView::ImageDocumentState state;
        publishMetadata(state);

        beginSourceLoad(state, false);

        QVERIFY(state.embeddedMetadata().isEmpty());
    }

    {
        KiriView::ImageDocumentState state;
        publishMetadata(state);

        finishEmptySourceLoad(state);

        QVERIFY(state.embeddedMetadata().isEmpty());
    }

    {
        KiriView::ImageDocumentState state;
        publishMetadata(state);
        state.setSourceUrl(localUrl(QStringLiteral("/images/missing.png")));
        state.setLoading(true);
        state.setStatus(KiriView::ImageDocumentStatus::Loading);

        finishLoadWithError(
            state, loadSession(imageUrl, imageUrl), true, QStringLiteral("missing"));

        QVERIFY(state.embeddedMetadata().isEmpty());
    }
}

void TestImageOpenWorkflow::stateChangesFollowWorkflowDeltaOrder()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));

    {
        std::vector<KiriView::ImageDocumentChange> changes;
        KiriView::ImageDocumentState state(
            [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });

        beginSourceLoad(state, false);

        QCOMPARE(changes.size(), std::size_t(3));
        QCOMPARE(changes.at(0), KiriView::ImageDocumentChange::EmbeddedMetadata);
        QCOMPARE(changes.at(1), KiriView::ImageDocumentChange::Loading);
        QCOMPARE(changes.at(2), KiriView::ImageDocumentChange::Status);
    }

    {
        std::vector<KiriView::ImageDocumentChange> changes;
        KiriView::ImageDocumentState state(
            [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });

        finishSuccessfulImageLoad(state, loadSession(imageUrl, imageUrl));

        QCOMPARE(changes.size(), std::size_t(4));
        QCOMPARE(changes.at(0), KiriView::ImageDocumentChange::SourceUrl);
        QCOMPARE(changes.at(1), KiriView::ImageDocumentChange::DisplayedUrl);
        QCOMPARE(changes.at(2), KiriView::ImageDocumentChange::WindowTitleFileName);
        QCOMPARE(changes.at(3), KiriView::ImageDocumentChange::Status);
    }

    {
        std::vector<KiriView::ImageDocumentChange> changes;
        KiriView::ImageDocumentState state(
            [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });
        const QUrl replacementUrl = localUrl(QStringLiteral("/images/missing.png"));
        state.setSourceUrl(replacementUrl);
        state.setLoading(true);
        state.setStatus(KiriView::ImageDocumentStatus::Loading);
        changes.clear();

        finishLoadWithError(
            state, loadSession(replacementUrl, replacementUrl), true, QStringLiteral("missing"));

        QCOMPARE(changes.size(), std::size_t(4));
        QCOMPARE(changes.at(0), KiriView::ImageDocumentChange::Loading);
        QCOMPARE(changes.at(1), KiriView::ImageDocumentChange::ErrorString);
        QCOMPARE(changes.at(2), KiriView::ImageDocumentChange::EmbeddedMetadata);
        QCOMPARE(changes.at(3), KiriView::ImageDocumentChange::Status);
    }
}

QTEST_GUILESS_MAIN(TestImageOpenWorkflow)

#include "test_imageopenworkflow.moc"
