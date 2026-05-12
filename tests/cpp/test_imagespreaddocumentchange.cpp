// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagespreadpresentationcontroller.h"

#include <QObject>
#include <QTest>

class TestImageSpreadDocumentChange : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void spreadDocumentChangePlanRoutesControllerSideEffects();
};

void TestImageSpreadDocumentChange::spreadDocumentChangePlanRoutesControllerSideEffects()
{
    const KiriView::ImageSpreadDocumentChangePlan errorPlan
        = KiriView::imageSpreadDocumentChangePlan(
            KiriView::ImageDocumentChange::ErrorString, false);
    QVERIFY(errorPlan.finishTransition);
    QVERIFY(!errorPlan.refreshSecondaryPage);
    QVERIFY(!errorPlan.notifyRightToLeftReading);

    const KiriView::ImageSpreadDocumentChangePlan emptyErrorPlan
        = KiriView::imageSpreadDocumentChangePlan(KiriView::ImageDocumentChange::ErrorString, true);
    QVERIFY(!emptyErrorPlan.finishTransition);

    const KiriView::ImageSpreadDocumentChangePlan pageNavigationPlan
        = KiriView::imageSpreadDocumentChangePlan(
            KiriView::ImageDocumentChange::PageNavigation, true);
    QVERIFY(!pageNavigationPlan.finishTransition);
    QVERIFY(pageNavigationPlan.refreshSecondaryPage);
    QVERIFY(pageNavigationPlan.notifyRightToLeftReading);

    const KiriView::ImageSpreadDocumentChangePlan statusPlan
        = KiriView::imageSpreadDocumentChangePlan(KiriView::ImageDocumentChange::Status, false);
    QVERIFY(!statusPlan.finishTransition);
    QVERIFY(!statusPlan.refreshSecondaryPage);
    QVERIFY(!statusPlan.notifyRightToLeftReading);
}

QTEST_GUILESS_MAIN(TestImageSpreadDocumentChange)

#include "test_imagespreaddocumentchange.moc"
