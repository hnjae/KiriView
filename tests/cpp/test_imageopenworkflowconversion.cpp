// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/imageopenworkflowconversion.h"

#include <QObject>
#include <QTest>

class TestImageOpenWorkflowConversion : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sourceLoadPolicyInputMapsPlainFields();
    void sourceLoadPlanMapsBridgeOperationsToDomainEffects();
};

void TestImageOpenWorkflowConversion::sourceLoadPolicyInputMapsPlainFields()
{
    kiriview::Bridge::ImageDocumentSourceLoadPolicyInput input;
    input.loadKind = kiriview::Bridge::ImageDocumentSourceLoadKind::ReplacementSource;
    input.preserveTwoPageSpreadTransition = true;
    input.rightToLeftReadingEnabled = true;
    input.sourceWithinDisplayedComicBookArchive = true;
    input.hasRequestedContainerNavigationUrl = true;

    const kiriview::RustImageDocumentSourceLoadPolicyInput converted
        = kiriview::rustImageDocumentSourceLoadPolicyInput(input);

    QCOMPARE(converted.load_kind, kiriview::RustImageDocumentSourceLoadKind::ReplacementSource);
    QVERIFY(converted.preserve_two_page_spread_transition);
    QVERIFY(converted.right_to_left_reading_enabled);
    QVERIFY(converted.source_within_displayed_comic_book_archive);
    QVERIFY(converted.has_requested_container_navigation_url);
}

void TestImageOpenWorkflowConversion::sourceLoadPlanMapsBridgeOperationsToDomainEffects()
{
    using Effect = kiriview::ImageDocumentSourceLoadEffect;
    using RustOperation = kiriview::RustImageDocumentSourceLoadOperation;

    kiriview::RustImageDocumentSourceLoadPlan rustPlan {};
    rustPlan.operations.push_back(RustOperation::CancelFileDeletion);
    rustPlan.operations.push_back(RustOperation::CancelAllNavigation);
    rustPlan.operations.push_back(RustOperation::CancelPredecode);
    rustPlan.operations.push_back(RustOperation::FinishSpreadTransition);
    rustPlan.operations.push_back(RustOperation::ResetRightToLeftReading);
    rustPlan.operations.push_back(RustOperation::NotifyRightToLeftReadingChanged);
    rustPlan.operations.push_back(RustOperation::ClearSecondaryPage);
    rustPlan.operations.push_back(RustOperation::ClearLoadingContainerNavigationUrl);
    rustPlan.operations.push_back(RustOperation::SetLoadingContainerNavigationUrlToRequested);
    rustPlan.operations.push_back(RustOperation::SetContainerNavigationUrlToRequested);
    rustPlan.operations.push_back(RustOperation::PrepareSourceLoad);
    rustPlan.operations.push_back(RustOperation::SetSourceUrlToRequested);
    rustPlan.operations.push_back(RustOperation::BeginOpen);

    const kiriview::ImageDocumentSourceLoadPlan plan
        = kiriview::imageDocumentSourceLoadPlanFromBridge(rustPlan);

    const kiriview::ImageDocumentSourceLoadPlan expected {
        Effect::CancelFileDeletion,
        Effect::CancelAllNavigation,
        Effect::CancelPredecode,
        Effect::FinishSpreadTransition,
        Effect::ResetRightToLeftReading,
        Effect::NotifyRightToLeftReadingChanged,
        Effect::ClearSecondaryPage,
        Effect::ClearLoadingContainerNavigationUrl,
        Effect::SetLoadingContainerNavigationUrlToRequested,
        Effect::SetContainerNavigationUrlToRequested,
        Effect::PrepareSourceLoad,
        Effect::SetSourceUrlToRequested,
        Effect::BeginOpen,
    };
    QVERIFY(plan == expected);
}

QTEST_GUILESS_MAIN(TestImageOpenWorkflowConversion)

#include "test_imageopenworkflowconversion.moc"
