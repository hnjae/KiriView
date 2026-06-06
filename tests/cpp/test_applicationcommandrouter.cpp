// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationcommandrouter.h"

#include <QObject>
#include <QTest>

namespace {
using KiriView::ApplicationActions::ApplicationCommandRouter;
using KiriView::ApplicationActions::ApplicationCommandRouterInput;
using KiriView::ApplicationActions::ApplicationCommandRouterPorts;

struct CommandLog {
    bool imageAvailable = true;
    bool videoAvailable = true;
    bool videoSeekable = true;
    bool viewportHorizontallyPannable = false;
    bool scanMoved = false;
    int panCount = 0;
    double lastPanDx = 0.0;
    double lastPanDy = 0.0;
    int previousNavigationCount = 0;
    int nextNavigationCount = 0;
    int previousSinglePageCount = 0;
    int nextSinglePageCount = 0;
    int scanForwardCount = 0;
    int scanBackwardCount = 0;
    int nextDisplayedImageStartToFinalScanPositionCount = 0;
    int firstImageBoundaryCount = 0;
    int seekCount = 0;
    qint64 lastSeekDeltaMilliseconds = 0;
};

ApplicationCommandRouterPorts commandPorts(CommandLog &log)
{
    ApplicationCommandRouterPorts ports;
    ports.imageAvailable = [&log]() { return log.imageAvailable; };
    ports.imageViewportHorizontallyPannable = [&log]() { return log.viewportHorizontallyPannable; };
    ports.requestViewportPanBy = [&log](double dx, double dy) {
        ++log.panCount;
        log.lastPanDx = dx;
        log.lastPanDy = dy;
    };
    ports.requestViewportScanForward = [&log]() {
        ++log.scanForwardCount;
        return log.scanMoved;
    };
    ports.requestViewportScanBackward = [&log]() {
        ++log.scanBackwardCount;
        return log.scanMoved;
    };
    ports.requestNextDisplayedImageStartToFinalScanPosition
        = [&log]() { ++log.nextDisplayedImageStartToFinalScanPositionCount; };
    ports.openPreviousSinglePage = [&log]() { ++log.previousSinglePageCount; };
    ports.openNextSinglePage = [&log]() { ++log.nextSinglePageCount; };
    ports.requestPreviousActiveNavigationWithBoundary = [&log]() { ++log.previousNavigationCount; };
    ports.requestNextActiveNavigationWithBoundary = [&log]() { ++log.nextNavigationCount; };
    ports.showFirstImageBoundary = [&log]() { ++log.firstImageBoundaryCount; };
    ports.videoAvailable = [&log]() { return log.videoAvailable; };
    ports.videoSeekable = [&log]() { return log.videoSeekable; };
    ports.seekVideoBy = [&log](qint64 deltaMilliseconds) {
        ++log.seekCount;
        log.lastSeekDeltaMilliseconds = deltaMilliseconds;
    };
    return ports;
}
}

class TestApplicationCommandRouter : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void horizontalArrowsRouteImagePanAndNavigation();
    void videoHorizontalArrowsUseActiveNavigation();
    void singlePageAndVerticalPanRouteToImagePorts();
    void videoSeekChecksModeAndSeekability();
    void scanShortcutsRoutePolicyEffects();
};

void TestApplicationCommandRouter::horizontalArrowsRouteImagePanAndNavigation()
{
    ApplicationCommandRouter router;
    ApplicationCommandRouterInput input;
    CommandLog log;
    log.viewportHorizontallyPannable = true;
    ApplicationCommandRouterPorts ports = commandPorts(log);

    QVERIFY(router.executeHorizontalArrowShortcut(input, ports, true));
    QCOMPARE(log.panCount, 1);
    QCOMPARE(log.lastPanDx, -64.0);
    QCOMPARE(log.lastPanDy, 0.0);

    log = CommandLog {};
    ports = commandPorts(log);
    QVERIFY(router.executeHorizontalArrowShortcut(input, ports, true));
    QCOMPARE(log.previousNavigationCount, 1);
    QCOMPARE(log.nextNavigationCount, 0);

    QVERIFY(router.executeHorizontalArrowShortcut(input, ports, false));
    QCOMPARE(log.nextNavigationCount, 1);

    log = CommandLog {};
    input.rightToLeftReadingActive = true;
    ports = commandPorts(log);
    QVERIFY(router.executeHorizontalArrowShortcut(input, ports, true));
    QCOMPARE(log.nextNavigationCount, 1);
}

void TestApplicationCommandRouter::videoHorizontalArrowsUseActiveNavigation()
{
    ApplicationCommandRouter router;
    ApplicationCommandRouterInput input;
    input.videoMode = true;
    CommandLog log;
    log.imageAvailable = false;
    ApplicationCommandRouterPorts ports = commandPorts(log);

    QVERIFY(router.executeHorizontalArrowShortcut(input, ports, true));
    QVERIFY(router.executeHorizontalArrowShortcut(input, ports, false));
    QCOMPARE(log.previousNavigationCount, 1);
    QCOMPARE(log.nextNavigationCount, 1);
    QCOMPARE(log.panCount, 0);
}

void TestApplicationCommandRouter::singlePageAndVerticalPanRouteToImagePorts()
{
    ApplicationCommandRouter router;
    ApplicationCommandRouterInput input;
    CommandLog log;
    ApplicationCommandRouterPorts ports = commandPorts(log);

    QVERIFY(router.executeSinglePageArrowShortcut(input, ports, true));
    QCOMPARE(log.previousSinglePageCount, 1);

    input.rightToLeftReadingActive = true;
    QVERIFY(router.executeSinglePageArrowShortcut(input, ports, true));
    QCOMPARE(log.nextSinglePageCount, 1);

    QVERIFY(router.executeVerticalPanShortcut(input, ports, true));
    QCOMPARE(log.panCount, 1);
    QCOMPARE(log.lastPanDx, 0.0);
    QCOMPARE(log.lastPanDy, -64.0);

    QVERIFY(router.executeVerticalPanShortcut(input, ports, false));
    QCOMPARE(log.panCount, 2);
    QCOMPARE(log.lastPanDy, 64.0);
}

void TestApplicationCommandRouter::videoSeekChecksModeAndSeekability()
{
    ApplicationCommandRouter router;
    ApplicationCommandRouterInput input;
    CommandLog log;
    ApplicationCommandRouterPorts ports = commandPorts(log);

    QVERIFY(!router.executeVideoSeekShortcut(input, ports, 5000));
    QCOMPARE(log.seekCount, 0);

    input.videoMode = true;
    log.videoAvailable = false;
    QVERIFY(!router.executeVideoSeekShortcut(input, ports, 5000));

    log.videoAvailable = true;
    log.videoSeekable = false;
    QVERIFY(router.executeVideoSeekShortcut(input, ports, 5000));
    QCOMPARE(log.seekCount, 0);

    log.videoSeekable = true;
    QVERIFY(router.executeVideoSeekShortcut(input, ports, -45000));
    QCOMPARE(log.seekCount, 1);
    QCOMPARE(log.lastSeekDeltaMilliseconds, qint64(-45000));
}

void TestApplicationCommandRouter::scanShortcutsRoutePolicyEffects()
{
    ApplicationCommandRouter router;
    ApplicationCommandRouterInput input;
    input.imagePannable = true;
    CommandLog log;
    log.scanMoved = true;
    ApplicationCommandRouterPorts ports = commandPorts(log);

    router.handleScanForwardAction(input, ports);
    QCOMPARE(log.scanForwardCount, 1);
    QCOMPARE(log.nextNavigationCount, 0);

    log.scanMoved = false;
    router.handleScanForwardAction(input, ports);
    QCOMPARE(log.nextNavigationCount, 1);

    input.imageDocumentPageNavigationActive = true;
    input.atKnownFirstActiveNavigation = true;
    router.handleScanBackwardAction(input, ports);
    QCOMPARE(log.firstImageBoundaryCount, 1);

    input.atKnownFirstActiveNavigation = false;
    input.canOpenPreviousActiveNavigation = true;
    router.handleScanBackwardAction(input, ports);
    QCOMPARE(log.nextDisplayedImageStartToFinalScanPositionCount, 1);
    QCOMPARE(log.previousNavigationCount, 1);

    input.imagePannable = false;
    router.handleScanBackwardAction(input, ports);
    QCOMPARE(log.previousNavigationCount, 2);
}

QTEST_GUILESS_MAIN(TestApplicationCommandRouter)

#include "test_applicationcommandrouter.moc"
