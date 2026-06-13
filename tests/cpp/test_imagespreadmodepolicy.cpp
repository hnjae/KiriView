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
    QVERIFY(!kiriview::imageSpreadReadingControlsAvailable(
        kiriview::ImageSpreadReadingAvailability { false, true, true }));
    QVERIFY(!kiriview::imageSpreadReadingControlsAvailable(
        kiriview::ImageSpreadReadingAvailability { true, false, true }));
    QVERIFY(!kiriview::imageSpreadReadingControlsAvailable(
        kiriview::ImageSpreadReadingAvailability { true, true, false }));
    QVERIFY(kiriview::imageSpreadReadingControlsAvailable(
        kiriview::ImageSpreadReadingAvailability { true, true, true }));
}

void TestImageSpreadModePolicy::twoPageModeChangePlansToggleSideEffects()
{
    const kiriview::ImageSpreadTwoPageModeChange noChange
        = kiriview::imageSpreadTwoPageModeChange(true, true, true);
    QVERIFY(!noChange.changed);

    const kiriview::ImageSpreadTwoPageModeChange enable
        = kiriview::imageSpreadTwoPageModeChange(false, true, false);
    QVERIFY(enable.changed);
    QVERIFY(!enable.finishTransition);
    QVERIFY(!enable.clearSecondaryPage);
    QVERIFY(enable.refreshSecondaryPage);
    QVERIFY(enable.notifyTwoPageMode);

    const kiriview::ImageSpreadTwoPageModeChange disableHidden
        = kiriview::imageSpreadTwoPageModeChange(true, false, false);
    QVERIFY(disableHidden.changed);
    QVERIFY(disableHidden.finishTransition);
    QVERIFY(disableHidden.clearSecondaryPage);
    QVERIFY(disableHidden.refreshSecondaryPage);
    QVERIFY(disableHidden.notifyTwoPageMode);

    const kiriview::ImageSpreadTwoPageModeChange disableVisible
        = kiriview::imageSpreadTwoPageModeChange(true, false, true);
    QVERIFY(disableVisible.changed);
    QVERIFY(disableVisible.finishTransition);
    QVERIFY(disableVisible.clearSecondaryPage);
    QVERIFY(disableVisible.refreshSecondaryPage);
    QVERIFY(disableVisible.notifyTwoPageMode);
}

QTEST_GUILESS_MAIN(TestImageSpreadModePolicy)

#include "test_imagespreadmodepolicy.moc"
