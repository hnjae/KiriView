// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagesourceloadworkflow.h"

#include <QObject>
#include <QTest>
#include <vector>

class TestImageSourceLoadWorkflow : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void routesUnchangedAndReplacementLoads();
};

void TestImageSourceLoadWorkflow::routesUnchangedAndReplacementLoads()
{
    using Action = KiriView::ImageSourceLoadAction;

    KiriView::ImageSourceLoadRequest unchangedRequest;
    unchangedRequest.sourceUrlChanged = false;
    unchangedRequest.preserveTwoPageSpreadTransition = false;
    unchangedRequest.resetRightToLeftReading = true;
    unchangedRequest.rightToLeftReadingEnabled = true;
    unchangedRequest.containerNavigationUrlEmpty = false;
    const KiriView::ImageSourceLoadPlan unchanged
        = KiriView::ImageSourceLoadWorkflow::plan(unchangedRequest);
    const std::vector<Action> unchangedActions {
        Action::FinishSpreadTransition,
        Action::ResetRightToLeftReading,
        Action::NotifyRightToLeftReading,
        Action::ClearLoadingContainerNavigationUrl,
        Action::UpdateContainerNavigationUrl,
    };
    QVERIFY(unchanged.actions == unchangedActions);

    KiriView::ImageSourceLoadRequest replacementRequest;
    replacementRequest.sourceUrlChanged = true;
    replacementRequest.preserveTwoPageSpreadTransition = true;
    replacementRequest.resetRightToLeftReading = false;
    replacementRequest.rightToLeftReadingEnabled = true;
    replacementRequest.containerNavigationUrlEmpty = true;
    const KiriView::ImageSourceLoadPlan replacement
        = KiriView::ImageSourceLoadWorkflow::plan(replacementRequest);
    const std::vector<Action> replacementActions {
        Action::CancelNavigationAndPredecode,
        Action::ClearSecondaryPage,
        Action::SetLoadingContainerNavigationUrl,
        Action::SetSourceUrl,
        Action::BeginOpen,
    };
    QVERIFY(replacement.actions == replacementActions);

    replacementRequest.resetRightToLeftReading = true;
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
    QVERIFY(resettingReplacement.actions == resettingReplacementActions);
}

QTEST_GUILESS_MAIN(TestImageSourceLoadWorkflow)
#include "test_imagesourceloadworkflow.moc"
