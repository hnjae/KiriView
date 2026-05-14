// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagesourceloadworkflow.h"

#include <QObject>
#include <QTest>
#include <vector>

namespace {
void compareActions(const rust::Vec<KiriView::ImageSourceLoadAction> &actual,
    const std::vector<KiriView::ImageSourceLoadAction> &expected)
{
    QCOMPARE(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}
}

class TestImageSourceLoadWorkflow : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void derivesRightToLeftReadingActionsFromRuntimeSnapshot();
    void routesUnchangedAndReplacementLoads();
};

void TestImageSourceLoadWorkflow::derivesRightToLeftReadingActionsFromRuntimeSnapshot()
{
    using Action = KiriView::ImageSourceLoadAction;

    KiriView::ImageSourceLoadPolicyInput input;
    input.source_url_changed = false;
    input.preserve_two_page_spread_transition = true;
    input.container_navigation_url_empty = true;

    input.reset_right_to_left_reading = false;
    input.right_to_left_reading_enabled = false;
    compareActions(KiriView::ImageSourceLoadWorkflow::plan(input).actions,
        {
            Action::ClearLoadingContainerNavigationUrl,
        });

    input.reset_right_to_left_reading = false;
    input.right_to_left_reading_enabled = true;
    compareActions(KiriView::ImageSourceLoadWorkflow::plan(input).actions,
        {
            Action::ClearLoadingContainerNavigationUrl,
        });

    input.reset_right_to_left_reading = true;
    input.right_to_left_reading_enabled = false;
    compareActions(KiriView::ImageSourceLoadWorkflow::plan(input).actions,
        {
            Action::ResetRightToLeftReading,
            Action::ClearLoadingContainerNavigationUrl,
        });

    input.reset_right_to_left_reading = true;
    input.right_to_left_reading_enabled = true;
    compareActions(KiriView::ImageSourceLoadWorkflow::plan(input).actions,
        {
            Action::ResetRightToLeftReading,
            Action::NotifyRightToLeftReading,
            Action::ClearLoadingContainerNavigationUrl,
        });
}

void TestImageSourceLoadWorkflow::routesUnchangedAndReplacementLoads()
{
    using Action = KiriView::ImageSourceLoadAction;

    KiriView::ImageSourceLoadPolicyInput unchangedInput;
    unchangedInput.source_url_changed = false;
    unchangedInput.preserve_two_page_spread_transition = false;
    unchangedInput.reset_right_to_left_reading = true;
    unchangedInput.right_to_left_reading_enabled = true;
    unchangedInput.container_navigation_url_empty = false;
    const KiriView::ImageSourceLoadPlan unchanged
        = KiriView::ImageSourceLoadWorkflow::plan(unchangedInput);
    const std::vector<Action> unchangedActions {
        Action::FinishSpreadTransition,
        Action::ResetRightToLeftReading,
        Action::NotifyRightToLeftReading,
        Action::ClearLoadingContainerNavigationUrl,
        Action::UpdateContainerNavigationUrl,
    };
    compareActions(unchanged.actions, unchangedActions);

    KiriView::ImageSourceLoadPolicyInput replacementInput;
    replacementInput.source_url_changed = true;
    replacementInput.preserve_two_page_spread_transition = true;
    replacementInput.reset_right_to_left_reading = false;
    replacementInput.right_to_left_reading_enabled = true;
    replacementInput.container_navigation_url_empty = true;
    const KiriView::ImageSourceLoadPlan replacement
        = KiriView::ImageSourceLoadWorkflow::plan(replacementInput);
    const std::vector<Action> replacementActions {
        Action::CancelNavigationAndPredecode,
        Action::ClearSecondaryPage,
        Action::SetLoadingContainerNavigationUrl,
        Action::SetSourceUrl,
        Action::BeginOpen,
    };
    compareActions(replacement.actions, replacementActions);

    replacementInput.reset_right_to_left_reading = true;
    replacementInput.right_to_left_reading_enabled = false;
    const KiriView::ImageSourceLoadPlan inactiveResetReplacement
        = KiriView::ImageSourceLoadWorkflow::plan(replacementInput);
    const std::vector<Action> inactiveResetReplacementActions {
        Action::CancelNavigationAndPredecode,
        Action::ResetRightToLeftReading,
        Action::ClearSecondaryPage,
        Action::SetLoadingContainerNavigationUrl,
        Action::SetSourceUrl,
        Action::BeginOpen,
    };
    compareActions(inactiveResetReplacement.actions, inactiveResetReplacementActions);

    replacementInput.reset_right_to_left_reading = true;
    replacementInput.right_to_left_reading_enabled = true;
    const KiriView::ImageSourceLoadPlan resettingReplacement
        = KiriView::ImageSourceLoadWorkflow::plan(replacementInput);
    const std::vector<Action> resettingReplacementActions {
        Action::CancelNavigationAndPredecode,
        Action::ResetRightToLeftReading,
        Action::ClearSecondaryPage,
        Action::SetLoadingContainerNavigationUrl,
        Action::SetSourceUrl,
        Action::BeginOpen,
        Action::NotifyRightToLeftReading,
    };
    compareActions(resettingReplacement.actions, resettingReplacementActions);
}

QTEST_GUILESS_MAIN(TestImageSourceLoadWorkflow)
#include "test_imagesourceloadworkflow.moc"
