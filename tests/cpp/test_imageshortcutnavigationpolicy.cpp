// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imageshortcutnavigationpolicy.h"

#include <QObject>
#include <QTest>

namespace {
using Policy = kiriview::ImageShortcutNavigationPolicy;
using HorizontalAction = kiriview::ImageShortcutNavigationPolicy::HorizontalArrowAction;
using ScanAction = kiriview::ImageShortcutNavigationPolicy::ScanAction;
using SinglePageAction = kiriview::ImageShortcutNavigationPolicy::SinglePageArrowAction;
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

    QCOMPARE(policy.horizontalArrowAction(true, false, false),
        HorizontalAction::RequestPreviousActiveNavigation);
    QCOMPARE(policy.horizontalArrowAction(false, false, false),
        HorizontalAction::RequestNextActiveNavigation);
    QCOMPARE(policy.horizontalArrowAction(true, false, true),
        HorizontalAction::RequestNextActiveNavigation);
    QCOMPARE(policy.horizontalArrowAction(false, false, true),
        HorizontalAction::RequestPreviousActiveNavigation);
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

    QCOMPARE(
        policy.scanForwardAction(false, false), ScanAction::RequestNextActiveNavigationFromScan);
    QCOMPARE(policy.scanForwardAction(true, true), ScanAction::NoScanAction);
    QCOMPARE(
        policy.scanForwardAction(true, false), ScanAction::RequestNextActiveNavigationFromScan);
}

void TestImageShortcutNavigationPolicy::scanBackwardSelectsBoundaryPageOrPreviousImage()
{
    Policy policy;

    QCOMPARE(policy.scanBackwardAction(false, false, false, false, true),
        ScanAction::RequestPreviousActiveNavigationFromScan);
    QCOMPARE(policy.scanBackwardAction(true, true, true, false, true), ScanAction::NoScanAction);
    QCOMPARE(policy.scanBackwardAction(true, false, true, true, false),
        ScanAction::ShowFirstImageBoundary);
    QCOMPARE(policy.scanBackwardAction(true, false, true, false, true),
        ScanAction::OpenPreviousPageFromFinalScanStart);
    QCOMPARE(policy.scanBackwardAction(true, false, false, false, true),
        ScanAction::RequestPreviousActiveNavigationFromScan);
}

QTEST_GUILESS_MAIN(TestImageShortcutNavigationPolicy)

#include "test_imageshortcutnavigationpolicy.moc"
