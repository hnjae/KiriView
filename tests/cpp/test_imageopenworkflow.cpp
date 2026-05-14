// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflow.h"

#include "image_test_support.h"
#include "imagecontainer.h"
#include "imagedocumentstate.h"

#include <QObject>
#include <QTest>
#include <QUrl>
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
    return KiriView::ImageLoadSession {
        1,
        KiriView::ImageLoadRequest::fromLocation(
            sourceUrl, archiveDocument, containerNavigationUrl),
        KiriView::DisplayedImageLocation::fromUrl(imageUrl, archiveDocument),
    };
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

void compareSourceLoadActions(const std::vector<KiriView::ImageSourceLoadAction> &actual,
    const std::vector<KiriView::ImageSourceLoadAction> &expected)
{
    QCOMPARE(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}
}

class TestImageOpenWorkflow : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void derivesSourceLoadRightToLeftReadingActionsFromRuntimeSnapshot();
    void routesUnchangedAndReplacementSourceLoads();
    void firstImageLoadSuccessTransitionsToReady();
    void directArchiveImageLoadSuccessDisablesContainerNavigation();
    void replacementLoadFailureKeepsDisplayedImage();
    void emptyContainerFailureSelectsFailedContainer();
    void animationFailureClearsImageAndResetsZoom();
    void routedLoadFailureAppliesErrorTransitions();
    void trackedLoadCompletionsClearLoadingContainerNavigationUrl();
};

void TestImageOpenWorkflow::derivesSourceLoadRightToLeftReadingActionsFromRuntimeSnapshot()
{
    using Action = KiriView::ImageSourceLoadAction;

    KiriView::ImageSourceLoadPolicyInput input;
    input.sourceUrlChanged = false;
    input.preserveTwoPageSpreadTransition = true;
    input.containerNavigationUrlEmpty = true;

    input.resetRightToLeftReading = false;
    input.rightToLeftReadingEnabled = false;
    compareSourceLoadActions(KiriView::ImageOpenWorkflow::sourceLoadPlan(input).actions,
        {
            Action::ClearLoadingContainerNavigationUrl,
        });

    input.resetRightToLeftReading = false;
    input.rightToLeftReadingEnabled = true;
    compareSourceLoadActions(KiriView::ImageOpenWorkflow::sourceLoadPlan(input).actions,
        {
            Action::ClearLoadingContainerNavigationUrl,
        });

    input.resetRightToLeftReading = true;
    input.rightToLeftReadingEnabled = false;
    compareSourceLoadActions(KiriView::ImageOpenWorkflow::sourceLoadPlan(input).actions,
        {
            Action::ResetRightToLeftReading,
            Action::ClearLoadingContainerNavigationUrl,
        });

    input.resetRightToLeftReading = true;
    input.rightToLeftReadingEnabled = true;
    compareSourceLoadActions(KiriView::ImageOpenWorkflow::sourceLoadPlan(input).actions,
        {
            Action::ResetRightToLeftReading,
            Action::NotifyRightToLeftReading,
            Action::ClearLoadingContainerNavigationUrl,
        });
}

void TestImageOpenWorkflow::routesUnchangedAndReplacementSourceLoads()
{
    using Action = KiriView::ImageSourceLoadAction;

    KiriView::ImageSourceLoadPolicyInput unchangedInput;
    unchangedInput.sourceUrlChanged = false;
    unchangedInput.preserveTwoPageSpreadTransition = false;
    unchangedInput.resetRightToLeftReading = true;
    unchangedInput.rightToLeftReadingEnabled = true;
    unchangedInput.containerNavigationUrlEmpty = false;
    const KiriView::ImageSourceLoadPlan unchanged
        = KiriView::ImageOpenWorkflow::sourceLoadPlan(unchangedInput);
    const std::vector<Action> unchangedActions {
        Action::FinishSpreadTransition,
        Action::ResetRightToLeftReading,
        Action::NotifyRightToLeftReading,
        Action::ClearLoadingContainerNavigationUrl,
        Action::UpdateContainerNavigationUrl,
    };
    compareSourceLoadActions(unchanged.actions, unchangedActions);

    KiriView::ImageSourceLoadPolicyInput replacementInput;
    replacementInput.sourceUrlChanged = true;
    replacementInput.preserveTwoPageSpreadTransition = true;
    replacementInput.resetRightToLeftReading = false;
    replacementInput.rightToLeftReadingEnabled = true;
    replacementInput.containerNavigationUrlEmpty = true;
    const KiriView::ImageSourceLoadPlan replacement
        = KiriView::ImageOpenWorkflow::sourceLoadPlan(replacementInput);
    const std::vector<Action> replacementActions {
        Action::CancelNavigationAndPredecode,
        Action::ClearSecondaryPage,
        Action::SetLoadingContainerNavigationUrl,
        Action::SetSourceUrl,
        Action::BeginOpen,
    };
    compareSourceLoadActions(replacement.actions, replacementActions);

    replacementInput.resetRightToLeftReading = true;
    replacementInput.rightToLeftReadingEnabled = false;
    const KiriView::ImageSourceLoadPlan inactiveResetReplacement
        = KiriView::ImageOpenWorkflow::sourceLoadPlan(replacementInput);
    const std::vector<Action> inactiveResetReplacementActions {
        Action::CancelNavigationAndPredecode,
        Action::ResetRightToLeftReading,
        Action::ClearSecondaryPage,
        Action::SetLoadingContainerNavigationUrl,
        Action::SetSourceUrl,
        Action::BeginOpen,
    };
    compareSourceLoadActions(inactiveResetReplacement.actions, inactiveResetReplacementActions);

    replacementInput.resetRightToLeftReading = true;
    replacementInput.rightToLeftReadingEnabled = true;
    const KiriView::ImageSourceLoadPlan resettingReplacement
        = KiriView::ImageOpenWorkflow::sourceLoadPlan(replacementInput);
    const std::vector<Action> resettingReplacementActions {
        Action::CancelNavigationAndPredecode,
        Action::ResetRightToLeftReading,
        Action::ClearSecondaryPage,
        Action::SetLoadingContainerNavigationUrl,
        Action::SetSourceUrl,
        Action::BeginOpen,
        Action::NotifyRightToLeftReading,
    };
    compareSourceLoadActions(resettingReplacement.actions, resettingReplacementActions);
}

void TestImageOpenWorkflow::firstImageLoadSuccessTransitionsToReady()
{
    KiriView::ImageDocumentState state;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    state.setSourceUrl(imageUrl);

    const KiriView::ImageDocumentEffects beginEffects
        = KiriView::ImageOpenWorkflow::beginSourceLoad(state, false);
    QVERIFY(hasEffect<KiriView::ClearImageEffect>(beginEffects));
    QVERIFY(hasEffect<KiriView::ResetZoomEffect>(beginEffects));
    QCOMPARE(beginEffects.size(), 2);
    QVERIFY(effectAt<KiriView::ClearImageEffect>(beginEffects, 0));
    QVERIFY(effectAt<KiriView::ResetZoomEffect>(beginEffects, 1));
    QVERIFY(state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Loading);

    const KiriView::ImageDocumentEffects successEffects
        = KiriView::ImageOpenWorkflow::finishSuccessfulImageLoad(
            state, loadSession(imageUrl, imageUrl));
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
        = KiriView::ImageOpenWorkflow::finishSuccessfulImageLoad(
            state, loadSession(archiveUrl, imageUrl, *archiveDocument));

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

    const KiriView::ImageDocumentEffects effects = KiriView::ImageOpenWorkflow::finishLoadWithError(
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
        = KiriView::ImageOpenWorkflow::finishContainerNavigationLoadWithError(
            state, containerUrl, QStringLiteral("empty"));
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
        = KiriView::ImageOpenWorkflow::finishAnimationLoadWithError(
            state, QStringLiteral("animation failed"));

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
            = KiriView::ImageOpenWorkflow::finishLoadWithError(
                state, containerNavigationSession, true, QStringLiteral("empty"));
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
            = KiriView::ImageOpenWorkflow::finishLoadWithError(
                state, imageSession, true, QStringLiteral("missing"));
        QVERIFY(!hasEffect<KiriView::ClearImageEffect>(effects));
        QVERIFY(hasEffect<KiriView::UpdatePageNavigationEffect>(effects));
        QCOMPARE(state.sourceUrl(), imageUrl);
        QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Ready);
    }

    {
        KiriView::ImageDocumentState state;
        state.setLoading(true);
        const KiriView::ImageDocumentEffects effects
            = KiriView::ImageOpenWorkflow::finishLoadWithError(
                state, imageSession, false, QStringLiteral("missing"));
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

        KiriView::ImageOpenWorkflow::finishEmptySourceLoad(state);

        QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    }

    {
        KiriView::ImageDocumentState state;
        state.setLoading(true);
        state.setLoadingContainerNavigationUrl(loadingContainerUrl);

        KiriView::ImageOpenWorkflow::finishSuccessfulImageLoad(
            state, loadSession(imageUrl, imageUrl));

        QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    }

    {
        KiriView::ImageDocumentState state;
        state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(imageUrl));
        state.setLoading(true);
        state.setLoadingContainerNavigationUrl(loadingContainerUrl);

        KiriView::ImageOpenWorkflow::finishLoadWithError(
            state, loadSession(imageUrl, imageUrl), true, QStringLiteral("missing"));

        QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    }

    {
        KiriView::ImageDocumentState state;
        state.setLoading(true);
        state.setLoadingContainerNavigationUrl(loadingContainerUrl);

        KiriView::ImageOpenWorkflow::finishLoadWithError(
            state, loadSession(imageUrl, imageUrl), false, QStringLiteral("missing"));

        QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    }

    {
        KiriView::ImageDocumentState state;
        state.setLoading(true);
        state.setLoadingContainerNavigationUrl(loadingContainerUrl);

        KiriView::ImageOpenWorkflow::finishContainerNavigationLoadWithError(
            state, loadingContainerUrl, QStringLiteral("missing"));

        QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    }

    {
        KiriView::ImageDocumentState state;
        state.setLoading(true);
        state.setLoadingContainerNavigationUrl(loadingContainerUrl);

        KiriView::ImageOpenWorkflow::finishAnimationLoadWithError(
            state, QStringLiteral("animation failed"));

        QVERIFY(state.loadingContainerNavigationUrl().isEmpty());
    }
}

QTEST_GUILESS_MAIN(TestImageOpenWorkflow)

#include "test_imageopenworkflow.moc"
