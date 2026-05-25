// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/imagespreadpolicyconversion.h"

#include <QObject>
#include <QTest>

class TestImageSpreadPolicyConversion : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void readingAvailabilityMapsFields();
    void twoPageModeChangeMapsFields();
    void secondaryRefreshMapsFieldsAndDecisions();
    void navigationMapsFieldsAndDirections();
};

void TestImageSpreadPolicyConversion::readingAvailabilityMapsFields()
{
    const KiriView::RustImageSpreadReadingAvailability rust
        = KiriView::Bridge::rustImageSpreadReadingAvailability(
            KiriView::ImageSpreadReadingAvailability { true, false, true });

    QVERIFY(rust.has_image);
    QVERIFY(!rust.has_displayed_image);
    QVERIFY(rust.displayed_document_is_comic_book);
}

void TestImageSpreadPolicyConversion::twoPageModeChangeMapsFields()
{
    const KiriView::ImageSpreadTwoPageModeChange change
        = KiriView::Bridge::imageSpreadTwoPageModeChangeFromRust(
            KiriView::RustImageSpreadTwoPageModeChange {
                true, false, true, false, true, false, true });

    QVERIFY(change.changed);
    QVERIFY(!change.resetSpreadZoom);
    QVERIFY(change.finishTransition);
    QVERIFY(!change.clearSecondaryPage);
    QVERIFY(change.restorePrimaryZoom);
    QVERIFY(!change.refreshSecondaryPage);
    QVERIFY(change.notifyTwoPageMode);
}

void TestImageSpreadPolicyConversion::secondaryRefreshMapsFieldsAndDecisions()
{
    const KiriView::RustImageSpreadSecondaryPageRefreshState rustState
        = KiriView::Bridge::rustImageSpreadSecondaryPageRefreshState(
            KiriView::ImageSpreadSecondaryPageRefreshState {
                true, 5, 9, false, true, false, true });
    QVERIFY(rustState.two_page_mode_active);
    QCOMPARE(rustState.current_page_number, 5);
    QCOMPARE(rustState.image_count, 9);
    QVERIFY(!rustState.primary_page_is_wide);
    QVERIFY(rustState.next_page_available);
    QVERIFY(!rustState.next_page_is_wide);
    QVERIFY(rustState.current_secondary_matches_next);

    QCOMPARE(KiriView::Bridge::imageSpreadSecondaryPageDecisionFromRust(
                 KiriView::RustImageSpreadSecondaryPageDecision::PrimaryOnly),
        KiriView::ImageSpreadSecondaryPageDecision::PrimaryOnly);
    QCOMPARE(KiriView::Bridge::imageSpreadSecondaryPageDecisionFromRust(
                 KiriView::RustImageSpreadSecondaryPageDecision::LoadNext),
        KiriView::ImageSpreadSecondaryPageDecision::LoadNext);
    QCOMPARE(KiriView::Bridge::imageSpreadSecondaryPageDecisionFromRust(
                 KiriView::RustImageSpreadSecondaryPageDecision::KeepCurrentSecondary),
        KiriView::ImageSpreadSecondaryPageDecision::KeepCurrentSecondary);

    const KiriView::ImageSpreadSecondaryPageRefreshPlan plan
        = KiriView::Bridge::imageSpreadSecondaryPageRefreshPlanFromRust(
            KiriView::RustImageSpreadSecondaryPageRefreshPlan {
                KiriView::RustImageSpreadSecondaryPageDecision::LoadNext,
                6,
            });
    QCOMPARE(plan.decision, KiriView::ImageSpreadSecondaryPageDecision::LoadNext);
    QCOMPARE(plan.targetPageNumber, 6);
}

void TestImageSpreadPolicyConversion::navigationMapsFieldsAndDirections()
{
    QCOMPARE(KiriView::Bridge::rustImageSpreadNavigationDirection(
                 KiriView::NavigationDirection::Previous),
        KiriView::RustImageSpreadNavigationDirection::Previous);
    QCOMPARE(
        KiriView::Bridge::rustImageSpreadNavigationDirection(KiriView::NavigationDirection::Next),
        KiriView::RustImageSpreadNavigationDirection::Next);

    const KiriView::RustImageSpreadNavigationState rustState
        = KiriView::Bridge::rustImageSpreadNavigationState(
            KiriView::ImageSpreadNavigationState { true, 3, 7, false, true });
    QVERIFY(rustState.two_page_mode_active);
    QCOMPARE(rustState.current_page_number, 3);
    QCOMPARE(rustState.image_count, 7);
    QVERIFY(!rustState.secondary_page_visible);
    QVERIFY(rustState.previous_page_is_wide);

    const KiriView::ImageSpreadPageNavigationTarget target
        = KiriView::Bridge::imageSpreadPageNavigationTargetFromRust(
            KiriView::RustImageSpreadPageNavigationTarget { true, 4 });
    QVERIFY(target.handledBySpread);
    QCOMPARE(target.pageNumber, 4);
}

QTEST_GUILESS_MAIN(TestImageSpreadPolicyConversion)

#include "test_imagespreadpolicyconversion.moc"
