// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imageopenworkflow.h"

#include "document/imagedocumentstate.h"
#include "document/imageopentransitionapplier.h"
#include "image_test_support.h"
#include "navigation/imagecontainer.h"

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
    const KiriView::ArchiveDocumentLocation &archiveDocument
    = KiriView::ArchiveDocumentLocation::none(),
    const QUrl &containerNavigationUrl = QUrl())
{
    return KiriView::ImageLoadSession(1,
        KiriView::ImageLoadRequest::fromLocation(
            sourceUrl, archiveDocument, containerNavigationUrl),
        KiriView::DisplayedImageLocation::fromUrl(imageUrl, archiveDocument));
}

template <typename Effect> const Effect *findEffect(const KiriView::ImageDocumentEffects &effects)
{
    for (const KiriView::ImageDocumentEffect &effect : effects) {
        if (const auto *payload = std::get_if<Effect>(&effect.payload)) {
            return payload;
        }
    }

    return nullptr;
}

template <typename Effect> bool hasEffect(const KiriView::ImageDocumentEffects &effects)
{
    return findEffect<Effect>(effects) != nullptr;
}

template <typename Effect>
bool effectAt(const KiriView::ImageDocumentEffects &effects, std::size_t index)
{
    if (index >= effects.size()) {
        return false;
    }

    return std::holds_alternative<Effect>(effects.at(index).payload);
}

bool transitionHasEffect(
    const KiriView::ImageOpenTransition &transition, KiriView::ImageOpenEffect effect)
{
    return std::find(transition.effects.cbegin(), transition.effects.cend(), effect)
        != transition.effects.cend();
}

KiriView::ImageDocumentEffects beginSourceLoad(KiriView::ImageDocumentState &state, bool hasImage)
{
    return KiriView::applyImageOpenTransition(state,
        KiriView::ImageOpenWorkflow::beginSourceLoadTransition(
            KiriView::ImageOpenBeginSourceLoadSnapshot {
                hasImage,
                !state.loadingContainerNavigationUrl().isEmpty(),
            }));
}

KiriView::ImageDocumentEffects finishEmptySourceLoad(KiriView::ImageDocumentState &state)
{
    return KiriView::applyImageOpenTransition(
        state, KiriView::ImageOpenWorkflow::finishEmptySourceLoadTransition());
}

KiriView::ImageDocumentEffects finishSuccessfulImageLoad(
    KiriView::ImageDocumentState &state, const KiriView::ImageLoadSession &session)
{
    return KiriView::applyImageOpenTransition(state,
        KiriView::ImageOpenWorkflow::finishSuccessfulImageLoadTransition(
            KiriView::ImageOpenSuccessfulImageLoadSnapshot {
                session.hasContainerNavigationTarget(),
            }),
        KiriView::ImageOpenTransitionContext::successfulImageLoad(session));
}

KiriView::ImageDocumentEffects finishLoadWithError(KiriView::ImageDocumentState &state,
    const KiriView::ImageLoadSession &session, bool hasImage, const QString &errorString)
{
    const QUrl displayedUrl = state.displayedUrl();
    return KiriView::applyImageOpenTransition(state,
        KiriView::ImageOpenWorkflow::finishLoadWithErrorTransition(
            KiriView::ImageOpenLoadErrorSnapshot {
                session.hasContainerNavigationTarget(),
                hasImage,
                !displayedUrl.isEmpty(),
            }),
        KiriView::ImageOpenTransitionContext::sourceLoadError(session, displayedUrl, errorString));
}

KiriView::ImageDocumentEffects finishContainerNavigationLoadWithError(
    KiriView::ImageDocumentState &state, const QUrl &containerUrl, const QString &errorString)
{
    return KiriView::applyImageOpenTransition(state,
        KiriView::ImageOpenWorkflow::finishContainerNavigationLoadWithErrorTransition(),
        KiriView::ImageOpenTransitionContext::containerNavigationError(containerUrl, errorString));
}

KiriView::ImageDocumentEffects finishAnimationLoadWithError(
    KiriView::ImageDocumentState &state, const QString &errorString)
{
    return KiriView::applyImageOpenTransition(state,
        KiriView::ImageOpenWorkflow::finishAnimationLoadWithErrorTransition(),
        KiriView::ImageOpenTransitionContext::animationError(errorString));
}

}

class TestImageOpenWorkflow : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void transitionsUseExplicitSnapshotInputs();
    void firstImageLoadSuccessTransitionsToReady();
    void directArchiveImageLoadSuccessDisablesContainerNavigation();
    void replacementLoadFailureKeepsDisplayedImage();
    void emptyContainerFailureSelectsFailedContainer();
    void animationFailureClearsImageAndResetsZoom();
    void routedLoadFailureAppliesErrorTransitions();
    void trackedLoadCompletionsClearLoadingContainerNavigationUrl();
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

    const KiriView::ImageOpenTransition replacementFailure
        = KiriView::ImageOpenWorkflow::finishLoadWithErrorTransition(
            KiriView::ImageOpenLoadErrorSnapshot { false, true, true });
    QCOMPARE(replacementFailure.stateDelta.sourceUrl, KiriView::ImageOpenUrlTarget::Displayed);
    QCOMPARE(replacementFailure.stateDelta.status, KiriView::ImageOpenStatusTarget::Ready);
    QVERIFY(
        transitionHasEffect(replacementFailure, KiriView::ImageOpenEffect::UpdatePageNavigation));
}

void TestImageOpenWorkflow::firstImageLoadSuccessTransitionsToReady()
{
    KiriView::ImageDocumentState state;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    state.setSourceUrl(imageUrl);

    const KiriView::ImageDocumentEffects beginEffects = beginSourceLoad(state, false);
    QVERIFY(hasEffect<KiriView::ClearImageEffect>(beginEffects));
    QVERIFY(hasEffect<KiriView::ResetZoomEffect>(beginEffects));
    QCOMPARE(beginEffects.size(), 2);
    QVERIFY(effectAt<KiriView::ClearImageEffect>(beginEffects, 0));
    QVERIFY(effectAt<KiriView::ResetZoomEffect>(beginEffects, 1));
    QVERIFY(state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Loading);

    const KiriView::ImageDocumentEffects successEffects
        = finishSuccessfulImageLoad(state, loadSession(imageUrl, imageUrl));
    QVERIFY(hasEffect<KiriView::UpdatePageNavigationEffect>(successEffects));
    QCOMPARE(successEffects.size(), 2);
    QVERIFY(effectAt<KiriView::UpdatePageNavigationEffect>(successEffects, 0));
    QVERIFY(effectAt<KiriView::ScheduleAdjacentImagePredecodeEffect>(successEffects, 1));
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
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl imageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));

    state.setSourceUrl(archiveUrl);

    const KiriView::ImageDocumentEffects successEffects
        = finishSuccessfulImageLoad(state, loadSession(archiveUrl, imageUrl, *archiveDocument));

    QVERIFY(hasEffect<KiriView::UpdatePageNavigationEffect>(successEffects));
    QCOMPARE(state.sourceUrl(), imageUrl);
    QCOMPARE(state.displayedUrl(), imageUrl);
    QCOMPARE(state.windowTitleFileName(), QStringLiteral("book.zip"));
    QVERIFY(state.containerNavigationUrl().isEmpty());
    QVERIFY(!state.containerNavigationAvailable());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Ready);
}

void TestImageOpenWorkflow::replacementLoadFailureKeepsDisplayedImage()
{
    KiriView::ImageDocumentState state;
    const QUrl displayedUrl = localUrl(QStringLiteral("/images/current.png"));
    const QUrl replacementUrl = localUrl(QStringLiteral("/images/missing.png"));
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(displayedUrl));
    state.setSourceUrl(replacementUrl);
    state.setLoading(true);
    state.setStatus(KiriView::ImageDocumentStatus::Ready);

    const KiriView::ImageDocumentEffects effects = finishLoadWithError(
        state, loadSession(replacementUrl, replacementUrl), true, QStringLiteral("missing"));
    QVERIFY(!hasEffect<KiriView::ClearImageEffect>(effects));
    QVERIFY(hasEffect<KiriView::UpdatePageNavigationEffect>(effects));
    QVERIFY(hasEffect<KiriView::ScheduleAdjacentImagePredecodeEffect>(effects));
    QCOMPARE(state.sourceUrl(), displayedUrl);
    QCOMPARE(state.displayedUrl(), displayedUrl);
    QCOMPARE(state.errorString(), QStringLiteral("missing"));
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Ready);
}

void TestImageOpenWorkflow::emptyContainerFailureSelectsFailedContainer()
{
    KiriView::ImageDocumentState state;
    const QUrl containerUrl = localUrl(QStringLiteral("/images/empty/"));
    state.setLoading(true);
    state.setLoadingContainerNavigationUrl(containerUrl);

    const KiriView::ImageDocumentEffects effects
        = finishContainerNavigationLoadWithError(state, containerUrl, QStringLiteral("empty"));
    QVERIFY(hasEffect<KiriView::ClearImageEffect>(effects));
    const auto *prepareFailedContainer
        = findEffect<KiriView::PrepareFailedContainerEffect>(effects);
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

    const KiriView::ImageDocumentEffects effects
        = finishAnimationLoadWithError(state, QStringLiteral("animation failed"));

    QVERIFY(hasEffect<KiriView::ClearImageEffect>(effects));
    QVERIFY(hasEffect<KiriView::ResetZoomEffect>(effects));
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
        containerUrl, imageUrl, KiriView::ArchiveDocumentLocation::none(), containerUrl);
    const KiriView::ImageLoadSession imageSession = loadSession(imageUrl, imageUrl);

    {
        KiriView::ImageDocumentState state;
        state.setLoading(true);
        const KiriView::ImageDocumentEffects effects
            = finishLoadWithError(state, containerNavigationSession, true, QStringLiteral("empty"));
        QVERIFY(hasEffect<KiriView::ClearImageEffect>(effects));
        QVERIFY(hasEffect<KiriView::PrepareFailedContainerEffect>(effects));
        QCOMPARE(state.sourceUrl(), containerUrl);
        QCOMPARE(state.containerNavigationUrl(), containerUrl);
        QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Error);
    }

    {
        KiriView::ImageDocumentState state;
        state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(imageUrl));
        state.setSourceUrl(localUrl(QStringLiteral("/images/missing.png")));
        state.setLoading(true);
        state.setStatus(KiriView::ImageDocumentStatus::Ready);
        const KiriView::ImageDocumentEffects effects
            = finishLoadWithError(state, imageSession, true, QStringLiteral("missing"));
        QVERIFY(!hasEffect<KiriView::ClearImageEffect>(effects));
        QVERIFY(hasEffect<KiriView::UpdatePageNavigationEffect>(effects));
        QCOMPARE(state.sourceUrl(), imageUrl);
        QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Ready);
    }

    {
        KiriView::ImageDocumentState state;
        state.setLoading(true);
        const KiriView::ImageDocumentEffects effects
            = finishLoadWithError(state, imageSession, false, QStringLiteral("missing"));
        QVERIFY(hasEffect<KiriView::ClearImageEffect>(effects));
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
        state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(imageUrl));
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

void TestImageOpenWorkflow::stateChangesFollowWorkflowDeltaOrder()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));

    {
        std::vector<KiriView::ImageDocumentChange> changes;
        KiriView::ImageDocumentState state(
            [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });

        beginSourceLoad(state, false);

        QCOMPARE(changes.size(), std::size_t(2));
        QCOMPARE(changes.at(0), KiriView::ImageDocumentChange::Loading);
        QCOMPARE(changes.at(1), KiriView::ImageDocumentChange::Status);
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
        const QUrl displayedUrl = localUrl(QStringLiteral("/images/current.png"));
        const QUrl replacementUrl = localUrl(QStringLiteral("/images/missing.png"));
        state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(displayedUrl));
        state.setSourceUrl(replacementUrl);
        state.setLoading(true);
        state.setStatus(KiriView::ImageDocumentStatus::Ready);
        changes.clear();

        finishLoadWithError(
            state, loadSession(replacementUrl, replacementUrl), true, QStringLiteral("missing"));

        QCOMPARE(changes.size(), std::size_t(3));
        QCOMPARE(changes.at(0), KiriView::ImageDocumentChange::Loading);
        QCOMPARE(changes.at(1), KiriView::ImageDocumentChange::SourceUrl);
        QCOMPARE(changes.at(2), KiriView::ImageDocumentChange::ErrorString);
    }
}

QTEST_GUILESS_MAIN(TestImageOpenWorkflow)

#include "test_imageopenworkflow.moc"
