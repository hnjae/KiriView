// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentsourceloadpolicy.h"

#include <QObject>
#include <QTest>

namespace {
void compareSourceLoadPlans(const KiriView::ImageDocumentSourceLoadPlan &actual,
    const KiriView::ImageDocumentSourceLoadPlan &expected)
{
    QCOMPARE(actual.cancelNavigationAndPredecode, expected.cancelNavigationAndPredecode);
    QCOMPARE(actual.finishSpreadTransition, expected.finishSpreadTransition);
    QCOMPARE(actual.rightToLeftReadingTransition, expected.rightToLeftReadingTransition);
    QCOMPARE(actual.clearSecondaryPage, expected.clearSecondaryPage);
    QCOMPARE(actual.loadingContainerNavigationUrl, expected.loadingContainerNavigationUrl);
    QCOMPARE(actual.containerNavigationUrl, expected.containerNavigationUrl);
    QCOMPARE(actual.sourceUrl, expected.sourceUrl);
    QCOMPARE(actual.beginOpen, expected.beginOpen);
}
}

class TestImageDocumentSourceLoadPolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sourceLoadPlanUsesRightToLeftReadingSnapshot();
    void sourceLoadPlanRoutesUnchangedAndReplacementSourceLoads();
};

void TestImageDocumentSourceLoadPolicy::sourceLoadPlanUsesRightToLeftReadingSnapshot()
{
    using ReadingTransition = KiriView::ImageDocumentRightToLeftReadingTransition;
    using UrlTarget = KiriView::ImageDocumentSourceLoadUrlTarget;

    KiriView::ImageDocumentSourceLoadPolicyInput input;
    input.loadKind = KiriView::ImageDocumentSourceLoadKind::CurrentSource;
    input.preserveTwoPageSpreadTransition = true;
    input.hasRequestedContainerNavigationUrl = false;

    input.rightToLeftReadingReset = KiriView::ImageDocumentRightToLeftReadingReset::Keep;
    compareSourceLoadPlans(KiriView::ImageDocumentSourceLoadPolicy::plan(input),
        KiriView::ImageDocumentSourceLoadPlan { false, false, ReadingTransition::Keep, false,
            UrlTarget::Empty, UrlTarget::Unchanged, UrlTarget::Unchanged, false });

    input.rightToLeftReadingReset = KiriView::ImageDocumentRightToLeftReadingReset::ResetInactive;
    compareSourceLoadPlans(KiriView::ImageDocumentSourceLoadPolicy::plan(input),
        KiriView::ImageDocumentSourceLoadPlan { false, false, ReadingTransition::Reset, false,
            UrlTarget::Empty, UrlTarget::Unchanged, UrlTarget::Unchanged, false });

    input.rightToLeftReadingReset = KiriView::ImageDocumentRightToLeftReadingReset::ResetActive;
    compareSourceLoadPlans(KiriView::ImageDocumentSourceLoadPolicy::plan(input),
        KiriView::ImageDocumentSourceLoadPlan { false, false,
            ReadingTransition::ResetAndNotifyBeforeSourceState, false, UrlTarget::Empty,
            UrlTarget::Unchanged, UrlTarget::Unchanged, false });
}

void TestImageDocumentSourceLoadPolicy::sourceLoadPlanRoutesUnchangedAndReplacementSourceLoads()
{
    using ReadingTransition = KiriView::ImageDocumentRightToLeftReadingTransition;
    using UrlTarget = KiriView::ImageDocumentSourceLoadUrlTarget;

    KiriView::ImageDocumentSourceLoadPolicyInput unchangedInput;
    unchangedInput.loadKind = KiriView::ImageDocumentSourceLoadKind::CurrentSource;
    unchangedInput.preserveTwoPageSpreadTransition = false;
    unchangedInput.rightToLeftReadingReset
        = KiriView::ImageDocumentRightToLeftReadingReset::ResetActive;
    unchangedInput.hasRequestedContainerNavigationUrl = true;
    const KiriView::ImageDocumentSourceLoadPlan unchanged
        = KiriView::ImageDocumentSourceLoadPolicy::plan(unchangedInput);
    compareSourceLoadPlans(unchanged,
        KiriView::ImageDocumentSourceLoadPlan { false, true,
            ReadingTransition::ResetAndNotifyBeforeSourceState, false, UrlTarget::Empty,
            UrlTarget::RequestedContainerNavigation, UrlTarget::Unchanged, false });

    KiriView::ImageDocumentSourceLoadPolicyInput replacementInput;
    replacementInput.loadKind = KiriView::ImageDocumentSourceLoadKind::ReplacementSource;
    replacementInput.preserveTwoPageSpreadTransition = true;
    replacementInput.rightToLeftReadingReset = KiriView::ImageDocumentRightToLeftReadingReset::Keep;
    replacementInput.hasRequestedContainerNavigationUrl = false;
    const KiriView::ImageDocumentSourceLoadPlan replacement
        = KiriView::ImageDocumentSourceLoadPolicy::plan(replacementInput);
    compareSourceLoadPlans(replacement,
        KiriView::ImageDocumentSourceLoadPlan { true, false, ReadingTransition::Keep, true,
            UrlTarget::RequestedContainerNavigation, UrlTarget::Unchanged,
            UrlTarget::RequestedSource, true });

    replacementInput.rightToLeftReadingReset
        = KiriView::ImageDocumentRightToLeftReadingReset::ResetInactive;
    const KiriView::ImageDocumentSourceLoadPlan inactiveResetReplacement
        = KiriView::ImageDocumentSourceLoadPolicy::plan(replacementInput);
    compareSourceLoadPlans(inactiveResetReplacement,
        KiriView::ImageDocumentSourceLoadPlan { true, false, ReadingTransition::Reset, true,
            UrlTarget::RequestedContainerNavigation, UrlTarget::Unchanged,
            UrlTarget::RequestedSource, true });

    replacementInput.rightToLeftReadingReset
        = KiriView::ImageDocumentRightToLeftReadingReset::ResetActive;
    const KiriView::ImageDocumentSourceLoadPlan resettingReplacement
        = KiriView::ImageDocumentSourceLoadPolicy::plan(replacementInput);
    compareSourceLoadPlans(resettingReplacement,
        KiriView::ImageDocumentSourceLoadPlan { true, false,
            ReadingTransition::ResetAndNotifyAfterOpen, true,
            UrlTarget::RequestedContainerNavigation, UrlTarget::Unchanged,
            UrlTarget::RequestedSource, true });
}

QTEST_GUILESS_MAIN(TestImageDocumentSourceLoadPolicy)

#include "test_imagedocumentsourceloadpolicy.moc"
