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
    const kiriview::RustImageSpreadReadingAvailability rust
        = kiriview::Bridge::rustImageSpreadReadingAvailability(
            kiriview::ImageSpreadReadingAvailability { true, false, true });

    QVERIFY(rust.has_image);
    QVERIFY(!rust.has_displayed_image);
    QVERIFY(rust.displayed_document_is_comic_book);
}

void TestImageSpreadPolicyConversion::twoPageModeChangeMapsFields()
{
    const kiriview::ImageSpreadTwoPageModeChange change
        = kiriview::Bridge::imageSpreadTwoPageModeChangeFromRust(
            kiriview::RustImageSpreadTwoPageModeChange { true, true, false, false, true });

    QVERIFY(change.changed);
    QVERIFY(change.finishTransition);
    QVERIFY(!change.clearSecondaryPage);
    QVERIFY(!change.refreshSecondaryPage);
    QVERIFY(change.notifyTwoPageMode);
}

void TestImageSpreadPolicyConversion::secondaryRefreshMapsFieldsAndDecisions()
{
    const kiriview::RustImageSpreadSecondaryPageRefreshState rustState
        = kiriview::Bridge::rustImageSpreadSecondaryPageRefreshState(
            kiriview::ImageSpreadSecondaryPageRefreshState {
                true, 5, 9, false, true, false, true });
    QVERIFY(rustState.two_page_mode_active);
    QCOMPARE(rustState.current_page_number, 5);
    QCOMPARE(rustState.image_count, 9);
    QVERIFY(!rustState.primary_page_is_wide);
    QVERIFY(rustState.next_page_available);
    QVERIFY(!rustState.next_page_is_wide);
    QVERIFY(rustState.current_secondary_matches_next);

    QCOMPARE(kiriview::Bridge::imageSpreadSecondaryPageDecisionFromRust(
                 kiriview::RustImageSpreadSecondaryPageDecision::PrimaryOnly),
        kiriview::ImageSpreadSecondaryPageDecision::PrimaryOnly);
    QCOMPARE(kiriview::Bridge::imageSpreadSecondaryPageDecisionFromRust(
                 kiriview::RustImageSpreadSecondaryPageDecision::LoadNext),
        kiriview::ImageSpreadSecondaryPageDecision::LoadNext);
    QCOMPARE(kiriview::Bridge::imageSpreadSecondaryPageDecisionFromRust(
                 kiriview::RustImageSpreadSecondaryPageDecision::KeepCurrentSecondary),
        kiriview::ImageSpreadSecondaryPageDecision::KeepCurrentSecondary);

    const kiriview::ImageSpreadSecondaryPageRefreshPlan plan
        = kiriview::Bridge::imageSpreadSecondaryPageRefreshPlanFromRust(
            kiriview::RustImageSpreadSecondaryPageRefreshPlan {
                kiriview::RustImageSpreadSecondaryPageDecision::LoadNext,
                6,
            });
    QCOMPARE(plan.decision, kiriview::ImageSpreadSecondaryPageDecision::LoadNext);
    QCOMPARE(plan.targetPageNumber, 6);
}

void TestImageSpreadPolicyConversion::navigationMapsFieldsAndDirections()
{
    QCOMPARE(kiriview::Bridge::rustImageSpreadNavigationDirection(
                 kiriview::NavigationDirection::Previous),
        kiriview::RustImageSpreadNavigationDirection::Previous);
    QCOMPARE(
        kiriview::Bridge::rustImageSpreadNavigationDirection(kiriview::NavigationDirection::Next),
        kiriview::RustImageSpreadNavigationDirection::Next);

    const kiriview::RustImageSpreadNavigationState rustState
        = kiriview::Bridge::rustImageSpreadNavigationState(
            kiriview::ImageSpreadNavigationState { true, 3, 7, false, true });
    QVERIFY(rustState.two_page_mode_active);
    QCOMPARE(rustState.current_page_number, 3);
    QCOMPARE(rustState.image_count, 7);
    QVERIFY(!rustState.secondary_page_visible);
    QVERIFY(rustState.previous_page_is_wide);

    const kiriview::ImageSpreadPageNavigationTarget target
        = kiriview::Bridge::imageSpreadPageNavigationTargetFromRust(
            kiriview::RustImageSpreadPageNavigationTarget { true, 4 });
    QVERIFY(target.handledBySpread);
    QCOMPARE(target.pageNumber, 4);
}

QTEST_GUILESS_MAIN(TestImageSpreadPolicyConversion)

#include "test_imagespreadpolicyconversion.moc"
