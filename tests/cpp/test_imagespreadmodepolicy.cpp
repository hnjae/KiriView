// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadmodepolicy.h"

#include <QObject>
#include <QTest>

class TestImageSpreadModePolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void readingControlsRequireDisplayedComicArchiveImage();
    void twoPageModeChangePlansToggleSideEffects();
};

void TestImageSpreadModePolicy::readingControlsRequireDisplayedComicArchiveImage()
{
    QVERIFY(!KiriView::imageSpreadReadingControlsAvailable(
        KiriView::ImageSpreadReadingAvailability { false, true, true }));
    QVERIFY(!KiriView::imageSpreadReadingControlsAvailable(
        KiriView::ImageSpreadReadingAvailability { true, false, true }));
    QVERIFY(!KiriView::imageSpreadReadingControlsAvailable(
        KiriView::ImageSpreadReadingAvailability { true, true, false }));
    QVERIFY(KiriView::imageSpreadReadingControlsAvailable(
        KiriView::ImageSpreadReadingAvailability { true, true, true }));
}

void TestImageSpreadModePolicy::twoPageModeChangePlansToggleSideEffects()
{
    const KiriView::ImageSpreadTwoPageModeChange noChange
        = KiriView::imageSpreadTwoPageModeChange(true, true, true);
    QVERIFY(!noChange.changed);

    const KiriView::ImageSpreadTwoPageModeChange enable
        = KiriView::imageSpreadTwoPageModeChange(false, true, false);
    QVERIFY(enable.changed);
    QVERIFY(enable.resetSpreadZoom);
    QVERIFY(!enable.finishTransition);
    QVERIFY(!enable.clearSecondaryPage);
    QVERIFY(!enable.restorePrimaryZoom);
    QVERIFY(enable.refreshSecondaryPage);
    QVERIFY(enable.notifyTwoPageMode);

    const KiriView::ImageSpreadTwoPageModeChange disableHidden
        = KiriView::imageSpreadTwoPageModeChange(true, false, false);
    QVERIFY(disableHidden.changed);
    QVERIFY(!disableHidden.resetSpreadZoom);
    QVERIFY(disableHidden.finishTransition);
    QVERIFY(disableHidden.clearSecondaryPage);
    QVERIFY(!disableHidden.restorePrimaryZoom);
    QVERIFY(disableHidden.refreshSecondaryPage);
    QVERIFY(disableHidden.notifyTwoPageMode);

    const KiriView::ImageSpreadTwoPageModeChange disableVisible
        = KiriView::imageSpreadTwoPageModeChange(true, false, true);
    QVERIFY(disableVisible.restorePrimaryZoom);
}

QTEST_GUILESS_MAIN(TestImageSpreadModePolicy)

#include "test_imagespreadmodepolicy.moc"
