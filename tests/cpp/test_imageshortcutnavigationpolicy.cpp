// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imageshortcutnavigationpolicy.h"

#include <QObject>
#include <QTest>

namespace {
using Policy = KiriView::ImageShortcutNavigationPolicy;
using HorizontalAction = KiriView::ImageShortcutNavigationPolicy::HorizontalArrowAction;
using ScanAction = KiriView::ImageShortcutNavigationPolicy::ScanAction;
using SinglePageAction = KiriView::ImageShortcutNavigationPolicy::SinglePageArrowAction;
}

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
    Policy policy;

    QCOMPARE(policy.horizontalArrowAction(true, true, false), HorizontalAction::PanLeft);
    QCOMPARE(policy.horizontalArrowAction(false, true, true), HorizontalAction::PanRight);
}

void TestImageShortcutNavigationPolicy::horizontalNavigationFollowsReadingDirection()
{
    Policy policy;

    QCOMPARE(policy.horizontalArrowAction(true, false, false), HorizontalAction::OpenPreviousImage);
    QCOMPARE(policy.horizontalArrowAction(false, false, false), HorizontalAction::OpenNextImage);
    QCOMPARE(policy.horizontalArrowAction(true, false, true), HorizontalAction::OpenNextImage);
    QCOMPARE(policy.horizontalArrowAction(false, false, true), HorizontalAction::OpenPreviousImage);
}

void TestImageShortcutNavigationPolicy::singlePageArrowsFollowReadingDirection()
{
    Policy policy;

    QCOMPARE(policy.singlePageArrowAction(true, false), SinglePageAction::OpenPreviousSinglePage);
    QCOMPARE(policy.singlePageArrowAction(false, false), SinglePageAction::OpenNextSinglePage);
    QCOMPARE(policy.singlePageArrowAction(true, true), SinglePageAction::OpenNextSinglePage);
    QCOMPARE(policy.singlePageArrowAction(false, true), SinglePageAction::OpenPreviousSinglePage);
}

void TestImageShortcutNavigationPolicy::scanForwardFallsThroughToNextImageOnlyAtViewportEnd()
{
    Policy policy;

    QCOMPARE(policy.scanForwardAction(false, false), ScanAction::OpenNextImageFromScan);
    QCOMPARE(policy.scanForwardAction(true, true), ScanAction::NoScanAction);
    QCOMPARE(policy.scanForwardAction(true, false), ScanAction::OpenNextImageFromScan);
}

void TestImageShortcutNavigationPolicy::scanBackwardSelectsBoundaryPageOrPreviousImage()
{
    Policy policy;

    QCOMPARE(
        policy.scanBackwardAction(false, false, false, 3), ScanAction::OpenPreviousImageFromScan);
    QCOMPARE(policy.scanBackwardAction(true, true, false, 3), ScanAction::NoScanAction);
    QCOMPARE(policy.scanBackwardAction(true, false, true, 1), ScanAction::ShowFirstImageBoundary);
    QCOMPARE(policy.scanBackwardAction(true, false, false, 3),
        ScanAction::OpenPreviousPageFromFinalScanStart);
    QCOMPARE(
        policy.scanBackwardAction(true, false, false, 0), ScanAction::OpenPreviousImageFromScan);
}

QTEST_GUILESS_MAIN(TestImageShortcutNavigationPolicy)

#include "test_imageshortcutnavigationpolicy.moc"
