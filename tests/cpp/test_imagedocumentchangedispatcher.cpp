// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentchangedispatcher.h"

#include <QObject>
#include <QTest>

class TestImageDocumentChangeDispatcher : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void dispatchPlanRoutesControllerSideEffects();
};

void TestImageDocumentChangeDispatcher::dispatchPlanRoutesControllerSideEffects()
{
    const KiriView::ImageDocumentChangeDispatchPlan errorPlan
        = KiriView::imageDocumentChangeDispatchPlan(
            KiriView::ImageDocumentChange::ErrorString, false);
    QVERIFY(errorPlan.finishSpreadTransition);
    QVERIFY(!errorPlan.refreshSecondaryPage);
    QVERIFY(!errorPlan.notifyRightToLeftReading);

    const KiriView::ImageDocumentChangeDispatchPlan emptyErrorPlan
        = KiriView::imageDocumentChangeDispatchPlan(
            KiriView::ImageDocumentChange::ErrorString, true);
    QVERIFY(!emptyErrorPlan.finishSpreadTransition);

    const KiriView::ImageDocumentChangeDispatchPlan pageNavigationPlan
        = KiriView::imageDocumentChangeDispatchPlan(
            KiriView::ImageDocumentChange::PageNavigation, true);
    QVERIFY(!pageNavigationPlan.finishSpreadTransition);
    QVERIFY(pageNavigationPlan.refreshSecondaryPage);
    QVERIFY(pageNavigationPlan.notifyRightToLeftReading);

    const KiriView::ImageDocumentChangeDispatchPlan statusPlan
        = KiriView::imageDocumentChangeDispatchPlan(KiriView::ImageDocumentChange::Status, false);
    QVERIFY(!statusPlan.finishSpreadTransition);
    QVERIFY(!statusPlan.refreshSecondaryPage);
    QVERIFY(!statusPlan.notifyRightToLeftReading);
}

QTEST_GUILESS_MAIN(TestImageDocumentChangeDispatcher)

#include "test_imagedocumentchangedispatcher.moc"
