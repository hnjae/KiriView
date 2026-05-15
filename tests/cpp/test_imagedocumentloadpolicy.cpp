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
    input.loadKind = KiriView::ImageDocumentSourceLoadKind::CurrentSource;
    input.preserveTwoPageSpreadTransition = true;
    input.hasRequestedContainerNavigationUrl = false;

    input.rightToLeftReadingReset = KiriView::ImageDocumentRightToLeftReadingReset::Keep;
    compareSourceLoadActions(KiriView::ImageDocumentLoadPolicy::sourceLoadPlan(input).actions,
        {
            Action::ClearLoadingContainerNavigationUrl,
        });

    input.rightToLeftReadingReset = KiriView::ImageDocumentRightToLeftReadingReset::ResetInactive;
    compareSourceLoadActions(KiriView::ImageDocumentLoadPolicy::sourceLoadPlan(input).actions,
        {
            Action::ResetRightToLeftReading,
            Action::ClearLoadingContainerNavigationUrl,
        });

    input.rightToLeftReadingReset = KiriView::ImageDocumentRightToLeftReadingReset::ResetActive;
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
    unchangedInput.loadKind = KiriView::ImageDocumentSourceLoadKind::CurrentSource;
    unchangedInput.preserveTwoPageSpreadTransition = false;
    unchangedInput.rightToLeftReadingReset
        = KiriView::ImageDocumentRightToLeftReadingReset::ResetActive;
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
    replacementInput.loadKind = KiriView::ImageDocumentSourceLoadKind::ReplacementSource;
    replacementInput.preserveTwoPageSpreadTransition = true;
    replacementInput.rightToLeftReadingReset = KiriView::ImageDocumentRightToLeftReadingReset::Keep;
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

    replacementInput.rightToLeftReadingReset
        = KiriView::ImageDocumentRightToLeftReadingReset::ResetInactive;
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

    replacementInput.rightToLeftReadingReset
        = KiriView::ImageDocumentRightToLeftReadingReset::ResetActive;
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
