// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentloadpolicy.h"

#include <QObject>
#include <QTest>
#include <cstddef>
#include <vector>

namespace {
void compareSourceLoadActions(const std::vector<KiriView::ImageDocumentSourceLoadAction> &actual,
    const std::vector<KiriView::ImageDocumentSourceLoadAction> &expected)
{
    QCOMPARE(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}
}

class TestImageDocumentLoadPolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sourceLoadPlanUsesRightToLeftReadingSnapshot();
    void sourceLoadPlanRoutesUnchangedAndReplacementSourceLoads();
};

void TestImageDocumentLoadPolicy::sourceLoadPlanUsesRightToLeftReadingSnapshot()
{
    using Action = KiriView::ImageDocumentSourceLoadAction;

    KiriView::ImageDocumentSourceLoadPolicyInput input;
    input.replaceSource = false;
    input.preserveTwoPageSpreadTransition = true;
    input.hasRequestedContainerNavigationUrl = false;

    input.resetRightToLeftReading = false;
    input.rightToLeftReadingWasEnabled = false;
    compareSourceLoadActions(KiriView::ImageDocumentLoadPolicy::sourceLoadPlan(input).actions,
        {
            Action::ClearLoadingContainerNavigationUrl,
        });

    input.resetRightToLeftReading = false;
    input.rightToLeftReadingWasEnabled = true;
    compareSourceLoadActions(KiriView::ImageDocumentLoadPolicy::sourceLoadPlan(input).actions,
        {
            Action::ClearLoadingContainerNavigationUrl,
        });

    input.resetRightToLeftReading = true;
    input.rightToLeftReadingWasEnabled = false;
    compareSourceLoadActions(KiriView::ImageDocumentLoadPolicy::sourceLoadPlan(input).actions,
        {
            Action::ResetRightToLeftReading,
            Action::ClearLoadingContainerNavigationUrl,
        });

    input.resetRightToLeftReading = true;
    input.rightToLeftReadingWasEnabled = true;
    compareSourceLoadActions(KiriView::ImageDocumentLoadPolicy::sourceLoadPlan(input).actions,
        {
            Action::ResetRightToLeftReading,
            Action::NotifyRightToLeftReading,
            Action::ClearLoadingContainerNavigationUrl,
        });
}

void TestImageDocumentLoadPolicy::sourceLoadPlanRoutesUnchangedAndReplacementSourceLoads()
{
    using Action = KiriView::ImageDocumentSourceLoadAction;

    KiriView::ImageDocumentSourceLoadPolicyInput unchangedInput;
    unchangedInput.replaceSource = false;
    unchangedInput.preserveTwoPageSpreadTransition = false;
    unchangedInput.resetRightToLeftReading = true;
    unchangedInput.rightToLeftReadingWasEnabled = true;
    unchangedInput.hasRequestedContainerNavigationUrl = true;
    const KiriView::ImageDocumentSourceLoadPlan unchanged
        = KiriView::ImageDocumentLoadPolicy::sourceLoadPlan(unchangedInput);
    const std::vector<Action> unchangedActions {
        Action::FinishSpreadTransition,
        Action::ResetRightToLeftReading,
        Action::NotifyRightToLeftReading,
        Action::ClearLoadingContainerNavigationUrl,
        Action::UpdateContainerNavigationUrl,
    };
    compareSourceLoadActions(unchanged.actions, unchangedActions);

    KiriView::ImageDocumentSourceLoadPolicyInput replacementInput;
    replacementInput.replaceSource = true;
    replacementInput.preserveTwoPageSpreadTransition = true;
    replacementInput.resetRightToLeftReading = false;
    replacementInput.rightToLeftReadingWasEnabled = true;
    replacementInput.hasRequestedContainerNavigationUrl = false;
    const KiriView::ImageDocumentSourceLoadPlan replacement
        = KiriView::ImageDocumentLoadPolicy::sourceLoadPlan(replacementInput);
    const std::vector<Action> replacementActions {
        Action::CancelNavigationAndPredecode,
        Action::ClearSecondaryPage,
        Action::SetLoadingContainerNavigationUrl,
        Action::SetSourceUrl,
        Action::BeginOpen,
    };
    compareSourceLoadActions(replacement.actions, replacementActions);

    replacementInput.resetRightToLeftReading = true;
    replacementInput.rightToLeftReadingWasEnabled = false;
    const KiriView::ImageDocumentSourceLoadPlan inactiveResetReplacement
        = KiriView::ImageDocumentLoadPolicy::sourceLoadPlan(replacementInput);
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
    replacementInput.rightToLeftReadingWasEnabled = true;
    const KiriView::ImageDocumentSourceLoadPlan resettingReplacement
        = KiriView::ImageDocumentLoadPolicy::sourceLoadPlan(replacementInput);
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

QTEST_GUILESS_MAIN(TestImageDocumentLoadPolicy)

#include "test_imagedocumentloadpolicy.moc"
