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

    KiriView::ImageSourceLoadRequest request;
    request.source_url_changed = false;
    request.preserve_two_page_spread_transition = true;
    request.container_navigation_url_empty = true;

    request.reset_right_to_left_reading = false;
    request.right_to_left_reading_enabled = false;
    compareActions(KiriView::ImageSourceLoadWorkflow::plan(request).actions,
        {
            Action::ClearLoadingContainerNavigationUrl,
        });

    request.reset_right_to_left_reading = false;
    request.right_to_left_reading_enabled = true;
    compareActions(KiriView::ImageSourceLoadWorkflow::plan(request).actions,
        {
            Action::ClearLoadingContainerNavigationUrl,
        });

    request.reset_right_to_left_reading = true;
    request.right_to_left_reading_enabled = false;
    compareActions(KiriView::ImageSourceLoadWorkflow::plan(request).actions,
        {
            Action::ResetRightToLeftReading,
            Action::ClearLoadingContainerNavigationUrl,
        });

    request.reset_right_to_left_reading = true;
    request.right_to_left_reading_enabled = true;
    compareActions(KiriView::ImageSourceLoadWorkflow::plan(request).actions,
        {
            Action::ResetRightToLeftReading,
            Action::NotifyRightToLeftReading,
            Action::ClearLoadingContainerNavigationUrl,
        });
}

void TestImageSourceLoadWorkflow::routesUnchangedAndReplacementLoads()
{
    using Action = KiriView::ImageSourceLoadAction;

    KiriView::ImageSourceLoadRequest unchangedRequest;
    unchangedRequest.source_url_changed = false;
    unchangedRequest.preserve_two_page_spread_transition = false;
    unchangedRequest.reset_right_to_left_reading = true;
    unchangedRequest.right_to_left_reading_enabled = true;
    unchangedRequest.container_navigation_url_empty = false;
    const KiriView::ImageSourceLoadPlan unchanged
        = KiriView::ImageSourceLoadWorkflow::plan(unchangedRequest);
    const std::vector<Action> unchangedActions {
        Action::FinishSpreadTransition,
        Action::ResetRightToLeftReading,
        Action::NotifyRightToLeftReading,
        Action::ClearLoadingContainerNavigationUrl,
        Action::UpdateContainerNavigationUrl,
    };
    compareActions(unchanged.actions, unchangedActions);

    KiriView::ImageSourceLoadRequest replacementRequest;
    replacementRequest.source_url_changed = true;
    replacementRequest.preserve_two_page_spread_transition = true;
    replacementRequest.reset_right_to_left_reading = false;
    replacementRequest.right_to_left_reading_enabled = true;
    replacementRequest.container_navigation_url_empty = true;
    const KiriView::ImageSourceLoadPlan replacement
        = KiriView::ImageSourceLoadWorkflow::plan(replacementRequest);
    const std::vector<Action> replacementActions {
        Action::CancelNavigationAndPredecode,
        Action::ClearSecondaryPage,
        Action::SetLoadingContainerNavigationUrl,
        Action::SetSourceUrl,
        Action::BeginOpen,
    };
    compareActions(replacement.actions, replacementActions);

    replacementRequest.reset_right_to_left_reading = true;
    replacementRequest.right_to_left_reading_enabled = false;
    const KiriView::ImageSourceLoadPlan inactiveResetReplacement
        = KiriView::ImageSourceLoadWorkflow::plan(replacementRequest);
    const std::vector<Action> inactiveResetReplacementActions {
        Action::CancelNavigationAndPredecode,
        Action::ResetRightToLeftReading,
        Action::ClearSecondaryPage,
        Action::SetLoadingContainerNavigationUrl,
        Action::SetSourceUrl,
        Action::BeginOpen,
    };
    compareActions(inactiveResetReplacement.actions, inactiveResetReplacementActions);

    replacementRequest.reset_right_to_left_reading = true;
    replacementRequest.right_to_left_reading_enabled = true;
    const KiriView::ImageSourceLoadPlan resettingReplacement
        = KiriView::ImageSourceLoadWorkflow::plan(replacementRequest);
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
