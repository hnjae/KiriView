// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentsourceloadpolicy.h"

#include <QObject>
#include <QTest>
#include <cstddef>
#include <vector>

namespace {
void compareSourceLoadActions(const std::vector<KiriView::ImageSourceLoadAction> &actual,
    const std::vector<KiriView::ImageSourceLoadAction> &expected)
{
    QCOMPARE(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}
}

class TestImageDocumentSourceLoadPolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void derivesRightToLeftReadingActionsFromRuntimeSnapshot();
    void routesUnchangedAndReplacementSourceLoads();
};

void TestImageDocumentSourceLoadPolicy::derivesRightToLeftReadingActionsFromRuntimeSnapshot()
{
    using Action = KiriView::ImageSourceLoadAction;

    KiriView::ImageSourceLoadPolicyInput input;
    input.sourceUrlChanged = false;
    input.preserveTwoPageSpreadTransition = true;
    input.requestedContainerNavigationUrlEmpty = true;

    input.resetRightToLeftReading = false;
    input.rightToLeftReadingWasEnabled = false;
    compareSourceLoadActions(KiriView::ImageDocumentSourceLoadPolicy::sourceLoadPlan(input).actions,
        {
            Action::ClearLoadingContainerNavigationUrl,
        });

    input.resetRightToLeftReading = false;
    input.rightToLeftReadingWasEnabled = true;
    compareSourceLoadActions(KiriView::ImageDocumentSourceLoadPolicy::sourceLoadPlan(input).actions,
        {
            Action::ClearLoadingContainerNavigationUrl,
        });

    input.resetRightToLeftReading = true;
    input.rightToLeftReadingWasEnabled = false;
    compareSourceLoadActions(KiriView::ImageDocumentSourceLoadPolicy::sourceLoadPlan(input).actions,
        {
            Action::ResetRightToLeftReading,
            Action::ClearLoadingContainerNavigationUrl,
        });

    input.resetRightToLeftReading = true;
    input.rightToLeftReadingWasEnabled = true;
    compareSourceLoadActions(KiriView::ImageDocumentSourceLoadPolicy::sourceLoadPlan(input).actions,
        {
            Action::ResetRightToLeftReading,
            Action::NotifyRightToLeftReading,
            Action::ClearLoadingContainerNavigationUrl,
        });
}

void TestImageDocumentSourceLoadPolicy::routesUnchangedAndReplacementSourceLoads()
{
    using Action = KiriView::ImageSourceLoadAction;

    KiriView::ImageSourceLoadPolicyInput unchangedInput;
    unchangedInput.sourceUrlChanged = false;
    unchangedInput.preserveTwoPageSpreadTransition = false;
    unchangedInput.resetRightToLeftReading = true;
    unchangedInput.rightToLeftReadingWasEnabled = true;
    unchangedInput.requestedContainerNavigationUrlEmpty = false;
    const KiriView::ImageSourceLoadPlan unchanged
        = KiriView::ImageDocumentSourceLoadPolicy::sourceLoadPlan(unchangedInput);
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
    replacementInput.rightToLeftReadingWasEnabled = true;
    replacementInput.requestedContainerNavigationUrlEmpty = true;
    const KiriView::ImageSourceLoadPlan replacement
        = KiriView::ImageDocumentSourceLoadPolicy::sourceLoadPlan(replacementInput);
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
    const KiriView::ImageSourceLoadPlan inactiveResetReplacement
        = KiriView::ImageDocumentSourceLoadPolicy::sourceLoadPlan(replacementInput);
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
    const KiriView::ImageSourceLoadPlan resettingReplacement
        = KiriView::ImageDocumentSourceLoadPolicy::sourceLoadPlan(replacementInput);
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

QTEST_GUILESS_MAIN(TestImageDocumentSourceLoadPolicy)

#include "test_imagedocumentsourceloadpolicy.moc"
