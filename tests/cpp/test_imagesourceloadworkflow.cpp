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
    void derivesRightToLeftReadingChangeFromRuntimeSnapshot();
    void routesUnchangedAndReplacementLoads();
};

void TestImageSourceLoadWorkflow::derivesRightToLeftReadingChangeFromRuntimeSnapshot()
{
    using RightToLeftReadingChange = KiriView::ImageSourceLoadRightToLeftReadingChange;

    QCOMPARE(KiriView::ImageSourceLoadWorkflow::rightToLeftReadingChangeForLoad(false, false),
        RightToLeftReadingChange::None);
    QCOMPARE(KiriView::ImageSourceLoadWorkflow::rightToLeftReadingChangeForLoad(false, true),
        RightToLeftReadingChange::None);
    QCOMPARE(KiriView::ImageSourceLoadWorkflow::rightToLeftReadingChangeForLoad(true, false),
        RightToLeftReadingChange::Reset);
    QCOMPARE(KiriView::ImageSourceLoadWorkflow::rightToLeftReadingChangeForLoad(true, true),
        RightToLeftReadingChange::ResetAndNotify);
}

void TestImageSourceLoadWorkflow::routesUnchangedAndReplacementLoads()
{
    using Action = KiriView::ImageSourceLoadAction;
    using RightToLeftReadingChange = KiriView::ImageSourceLoadRightToLeftReadingChange;

    KiriView::ImageSourceLoadRequest unchangedRequest;
    unchangedRequest.sourceUrlChanged = false;
    unchangedRequest.preserveTwoPageSpreadTransition = false;
    unchangedRequest.rightToLeftReadingChange = RightToLeftReadingChange::ResetAndNotify;
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
    replacementRequest.rightToLeftReadingChange = RightToLeftReadingChange::None;
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

    replacementRequest.rightToLeftReadingChange = RightToLeftReadingChange::Reset;
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
    QVERIFY(inactiveResetReplacement.actions == inactiveResetReplacementActions);

    replacementRequest.rightToLeftReadingChange = RightToLeftReadingChange::ResetAndNotify;
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
