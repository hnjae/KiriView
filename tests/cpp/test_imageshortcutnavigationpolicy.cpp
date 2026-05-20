// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imageshortcutnavigationpolicy.h"

#include <QObject>
#include <QTest>

class TestImageShortcutNavigationPolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void horizontalArrowsPanBeforeNavigating();
    void horizontalNavigationFollowsReadingDirection();
    void singlePageArrowsFollowReadingDirection();
    void scanForwardFallsThroughToNextImageOnlyAtViewportEnd();
    void scanBackwardSelectsBoundaryPageOrPreviousImage();
};

void TestImageShortcutNavigationPolicy::horizontalArrowsPanBeforeNavigating()
{
    ImageShortcutNavigationPolicy policy;

    QCOMPARE(
        policy.horizontalArrowAction(true, true, false), ImageShortcutNavigationPolicy::PanLeft);
    QCOMPARE(
        policy.horizontalArrowAction(false, true, true), ImageShortcutNavigationPolicy::PanRight);
}

void TestImageShortcutNavigationPolicy::horizontalNavigationFollowsReadingDirection()
{
    ImageShortcutNavigationPolicy policy;

    QCOMPARE(policy.horizontalArrowAction(true, false, false),
        ImageShortcutNavigationPolicy::OpenPreviousImage);
    QCOMPARE(policy.horizontalArrowAction(false, false, false),
        ImageShortcutNavigationPolicy::OpenNextImage);
    QCOMPARE(policy.horizontalArrowAction(true, false, true),
        ImageShortcutNavigationPolicy::OpenNextImage);
    QCOMPARE(policy.horizontalArrowAction(false, false, true),
        ImageShortcutNavigationPolicy::OpenPreviousImage);
}

void TestImageShortcutNavigationPolicy::singlePageArrowsFollowReadingDirection()
{
    ImageShortcutNavigationPolicy policy;

    QCOMPARE(policy.singlePageArrowAction(true, false),
        ImageShortcutNavigationPolicy::OpenPreviousSinglePage);
    QCOMPARE(policy.singlePageArrowAction(false, false),
        ImageShortcutNavigationPolicy::OpenNextSinglePage);
    QCOMPARE(policy.singlePageArrowAction(true, true),
        ImageShortcutNavigationPolicy::OpenNextSinglePage);
    QCOMPARE(policy.singlePageArrowAction(false, true),
        ImageShortcutNavigationPolicy::OpenPreviousSinglePage);
}

void TestImageShortcutNavigationPolicy::scanForwardFallsThroughToNextImageOnlyAtViewportEnd()
{
    ImageShortcutNavigationPolicy policy;

    QCOMPARE(policy.scanForwardAction(false, false),
        ImageShortcutNavigationPolicy::OpenNextImageFromScan);
    QCOMPARE(policy.scanForwardAction(true, true), ImageShortcutNavigationPolicy::NoScanAction);
    QCOMPARE(policy.scanForwardAction(true, false),
        ImageShortcutNavigationPolicy::OpenNextImageFromScan);
}

void TestImageShortcutNavigationPolicy::scanBackwardSelectsBoundaryPageOrPreviousImage()
{
    ImageShortcutNavigationPolicy policy;

    QCOMPARE(policy.scanBackwardAction(false, false, false, 3),
        ImageShortcutNavigationPolicy::OpenPreviousImageFromScan);
    QCOMPARE(policy.scanBackwardAction(true, true, false, 3),
        ImageShortcutNavigationPolicy::NoScanAction);
    QCOMPARE(policy.scanBackwardAction(true, false, true, 1),
        ImageShortcutNavigationPolicy::ShowFirstImageBoundary);
    QCOMPARE(policy.scanBackwardAction(true, false, false, 3),
        ImageShortcutNavigationPolicy::OpenPreviousPageFromFinalScanStart);
    QCOMPARE(policy.scanBackwardAction(true, false, false, 0),
        ImageShortcutNavigationPolicy::OpenPreviousImageFromScan);
}

QTEST_GUILESS_MAIN(TestImageShortcutNavigationPolicy)

#include "test_imageshortcutnavigationpolicy.moc"
