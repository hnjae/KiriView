// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadsecondarypagerefreshpolicy.h"

#include <QObject>
#include <QTest>

class TestImageSpreadSecondaryPageRefreshPolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void secondaryPageRefreshPlanSelectsPrimaryKeepOrLoad();
};

void TestImageSpreadSecondaryPageRefreshPolicy::secondaryPageRefreshPlanSelectsPrimaryKeepOrLoad()
{
    using kiriview::ImageSpreadSecondaryPageDecision;

    const kiriview::ImageSpreadSecondaryPageRefreshPlan coverPlan
        = kiriview::imageSpreadSecondaryPageRefreshPlan(
            kiriview::ImageSpreadSecondaryPageRefreshState {
                true, 1, 4, false, true, false, false });
    QCOMPARE(coverPlan.decision, ImageSpreadSecondaryPageDecision::PrimaryOnly);
    QCOMPARE(coverPlan.targetPageNumber, 0);

    const kiriview::ImageSpreadSecondaryPageRefreshPlan widePrimaryPlan
        = kiriview::imageSpreadSecondaryPageRefreshPlan(
            kiriview::ImageSpreadSecondaryPageRefreshState {
                true, 2, 4, true, true, false, false });
    QCOMPARE(widePrimaryPlan.decision, ImageSpreadSecondaryPageDecision::PrimaryOnly);
    QCOMPARE(widePrimaryPlan.targetPageNumber, 0);

    const kiriview::ImageSpreadSecondaryPageRefreshPlan wideNextPlan
        = kiriview::imageSpreadSecondaryPageRefreshPlan(
            kiriview::ImageSpreadSecondaryPageRefreshState {
                true, 2, 4, false, true, true, false });
    QCOMPARE(wideNextPlan.decision, ImageSpreadSecondaryPageDecision::PrimaryOnly);
    QCOMPARE(wideNextPlan.targetPageNumber, 0);

    const kiriview::ImageSpreadSecondaryPageRefreshPlan keepPlan
        = kiriview::imageSpreadSecondaryPageRefreshPlan(
            kiriview::ImageSpreadSecondaryPageRefreshState {
                true, 2, 4, false, true, false, true });
    QCOMPARE(keepPlan.decision, ImageSpreadSecondaryPageDecision::KeepCurrentSecondary);
    QCOMPARE(keepPlan.targetPageNumber, 3);

    const kiriview::ImageSpreadSecondaryPageRefreshPlan loadPlan
        = kiriview::imageSpreadSecondaryPageRefreshPlan(
            kiriview::ImageSpreadSecondaryPageRefreshState {
                true, 2, 4, false, true, false, false });
    QCOMPARE(loadPlan.decision, ImageSpreadSecondaryPageDecision::LoadNext);
    QCOMPARE(loadPlan.targetPageNumber, 3);
}

QTEST_GUILESS_MAIN(TestImageSpreadSecondaryPageRefreshPolicy)

#include "test_imagespreadsecondarypagerefreshpolicy.moc"
