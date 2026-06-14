// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationactionstatepolicy.h"
#include "application/applicationcommandrouter.h"
#include "application/applicationtypes.h"
#include "application/applicationzoompresets.h"
#include "application/kiriviewapplicationactions.h"

#include <QObject>
#include <QStringList>
#include <QTest>

namespace {
using kiriview::ApplicationActions::ApplicationCommandRouter;
using kiriview::ApplicationActions::ApplicationCommandRouterInput;
using kiriview::ApplicationActions::ApplicationCommandRouterPorts;
using ActionId = kiriview::ApplicationActions::ActionId;

struct CommandLog {
    QStringList actionCalls;
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
    int setVideoPositionCount = 0;
    int toggleVideoPlaybackCount = 0;
    qint64 lastSeekDeltaMilliseconds = 0;
    qint64 videoDuration = 120000;
    qint64 lastVideoPosition = -1;
};

ApplicationCommandRouterPorts commandPorts(CommandLog &log)
{
    ApplicationCommandRouterPorts ports;
    ports.requestOpenDialog
        = [&log]() { log.actionCalls.push_back(QStringLiteral("open-dialog")); };
    ports.openCurrentMediaWith
        = [&log]() { log.actionCalls.push_back(QStringLiteral("open-with")); };
    ports.moveDisplayedFileToTrash
        = [&log]() { log.actionCalls.push_back(QStringLiteral("move-trash")); };
    ports.deleteDisplayedFilePermanently
        = [&log]() { log.actionCalls.push_back(QStringLiteral("delete-permanent")); };
    ports.imageAvailable = [&log]() { return log.imageAvailable; };
    ports.imageViewportHorizontallyPannable = [&log]() { return log.viewportHorizontallyPannable; };
    ports.requestViewportPanBy = [&log](double dx, double dy) {
        ++log.panCount;
        log.lastPanDx = dx;
        log.lastPanDy = dy;
    };
    ports.requestViewportScanForward = [&log]() {
        log.actionCalls.push_back(QStringLiteral("scan-forward"));
        ++log.scanForwardCount;
        return log.scanMoved;
    };
    ports.requestViewportScanBackward = [&log]() {
        log.actionCalls.push_back(QStringLiteral("scan-backward"));
        ++log.scanBackwardCount;
        return log.scanMoved;
    };
    ports.requestNextDisplayedImageStartToFinalScanPosition = [&log]() {
        log.actionCalls.push_back(
            QStringLiteral("next-displayed-image-start-to-final-scan-position"));
        ++log.nextDisplayedImageStartToFinalScanPositionCount;
    };
    ports.openPreviousContainer
        = [&log]() { log.actionCalls.push_back(QStringLiteral("previous-container")); };
    ports.openNextContainer
        = [&log]() { log.actionCalls.push_back(QStringLiteral("next-container")); };
    ports.openPreviousSinglePage = [&log]() {
        log.actionCalls.push_back(QStringLiteral("previous-single-page"));
        ++log.previousSinglePageCount;
    };
    ports.openNextSinglePage = [&log]() {
        log.actionCalls.push_back(QStringLiteral("next-single-page"));
        ++log.nextSinglePageCount;
    };
    ports.requestPreviousActiveNavigationWithBoundary = [&log]() {
        log.actionCalls.push_back(QStringLiteral("previous-navigation"));
        ++log.previousNavigationCount;
    };
    ports.requestNextActiveNavigationWithBoundary = [&log]() {
        log.actionCalls.push_back(QStringLiteral("next-navigation"));
        ++log.nextNavigationCount;
    };
    ports.openFirstActiveNavigation
        = [&log]() { log.actionCalls.push_back(QStringLiteral("first-navigation")); };
    ports.openLastActiveNavigation
        = [&log]() { log.actionCalls.push_back(QStringLiteral("last-navigation")); };
    ports.requestZoomByStepAtCenter = [&log](double stepCount) {
        log.actionCalls.push_back(QStringLiteral("zoom-step:%1").arg(stepCount));
    };
    ports.requestManualZoomPercent = [&log](double zoomPercent) {
        log.actionCalls.push_back(QStringLiteral("manual-zoom:%1").arg(zoomPercent));
    };
    ports.requestFitMode = [&log]() { log.actionCalls.push_back(QStringLiteral("fit")); };
    ports.requestFitHeightMode
        = [&log]() { log.actionCalls.push_back(QStringLiteral("fit-height")); };
    ports.requestFitWidthMode
        = [&log]() { log.actionCalls.push_back(QStringLiteral("fit-width")); };
    ports.rotateClockwise
        = [&log]() { log.actionCalls.push_back(QStringLiteral("rotate-clockwise")); };
    ports.rotateCounterclockwise
        = [&log]() { log.actionCalls.push_back(QStringLiteral("rotate-counterclockwise")); };
    ports.requestToggleTwoPageMode
        = [&log]() { log.actionCalls.push_back(QStringLiteral("toggle-two-page")); };
    ports.requestToggleRightToLeftReading
        = [&log]() { log.actionCalls.push_back(QStringLiteral("toggle-right-to-left")); };
    ports.toggleInfoPanel
        = [&log]() { log.actionCalls.push_back(QStringLiteral("toggle-info-panel")); };
    ports.toggleThumbnailPanel
        = [&log]() { log.actionCalls.push_back(QStringLiteral("toggle-thumbnail-panel")); };
    ports.requestViewportPanToInitialScanPosition
        = [&log]() { log.actionCalls.push_back(QStringLiteral("pan-initial-scan-position")); };
    ports.requestViewportPanToFinalScanPosition
        = [&log]() { log.actionCalls.push_back(QStringLiteral("pan-final-scan-position")); };
    ports.showFirstImageBoundary = [&log]() {
        log.actionCalls.push_back(QStringLiteral("first-image-boundary"));
        ++log.firstImageBoundaryCount;
    };
    ports.toggleFullScreen
        = [&log]() { log.actionCalls.push_back(QStringLiteral("toggle-fullscreen")); };
    ports.requestShortcutHelp
        = [&log]() { log.actionCalls.push_back(QStringLiteral("shortcut-help")); };
    ports.openApplicationMenu
        = [&log]() { log.actionCalls.push_back(QStringLiteral("open-application-menu")); };
    ports.videoAvailable = [&log]() { return log.videoAvailable; };
    ports.videoSeekable = [&log]() { return log.videoSeekable; };
    ports.videoDuration = [&log]() { return log.videoDuration; };
    ports.seekVideoBy = [&log](qint64 deltaMilliseconds) {
        ++log.seekCount;
        log.lastSeekDeltaMilliseconds = deltaMilliseconds;
    };
    ports.setVideoPosition = [&log](qint64 positionMilliseconds) {
        ++log.setVideoPositionCount;
        log.lastVideoPosition = positionMilliseconds;
    };
    ports.toggleVideoPlayback = [&log]() {
        log.actionCalls.push_back(QStringLiteral("toggle-video-playback"));
        ++log.toggleVideoPlaybackCount;
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
    void actionDispatchRoutesToPorts();
    void singlePageAndVerticalPanRouteToImagePorts();
    void videoPlaybackActionChecksModeAndVideoAvailability();
    void videoSeekChecksModeAndSeekability();
    void contentBoundaryActionsRouteByMediaMode();
    void zoomPresetDescriptorsOwnMetadataAndDispatch();
    void videoScanShortcutsUseActiveNavigation();
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

void TestApplicationCommandRouter::actionDispatchRoutesToPorts()
{
    ApplicationCommandRouter router;
    ApplicationCommandRouterInput input;
    CommandLog log;
    ApplicationCommandRouterPorts ports = commandPorts(log);

    router.handleActionTriggered(ActionId::FileOpenAction, input, ports);
    router.handleActionTriggered(ActionId::FileOpenWithAction, input, ports);
    router.handleActionTriggered(ActionId::FileMoveToTrashAction, input, ports);
    router.handleActionTriggered(ActionId::FileDeleteAction, input, ports);
    router.handleActionTriggered(ActionId::GoPreviousArchiveAction, input, ports);
    router.handleActionTriggered(ActionId::GoNextArchiveAction, input, ports);
    router.handleActionTriggered(ActionId::GoPreviousImageAction, input, ports);
    router.handleActionTriggered(ActionId::GoNextImageAction, input, ports);
    router.handleActionTriggered(ActionId::GoFirstImageAction, input, ports);
    router.handleActionTriggered(ActionId::GoLastImageAction, input, ports);
    router.handleActionTriggered(ActionId::ViewZoomInAction, input, ports);
    router.handleActionTriggered(ActionId::ViewZoomOutAction, input, ports);
    router.handleActionTriggered(ActionId::ViewZoom50PercentAction, input, ports);
    router.handleActionTriggered(ActionId::ViewZoom100PercentAction, input, ports);
    router.handleActionTriggered(ActionId::ViewZoom200PercentAction, input, ports);
    router.handleActionTriggered(ActionId::ViewFitAction, input, ports);
    router.handleActionTriggered(ActionId::ViewFitHeightAction, input, ports);
    router.handleActionTriggered(ActionId::ViewFitWidthAction, input, ports);
    router.handleActionTriggered(ActionId::ViewRotateClockwiseAction, input, ports);
    router.handleActionTriggered(ActionId::ViewRotateCounterclockwiseAction, input, ports);
    router.handleActionTriggered(ActionId::ViewToggleTwoPageModeAction, input, ports);
    router.handleActionTriggered(ActionId::ViewToggleRightToLeftReadingAction, input, ports);
    router.handleActionTriggered(ActionId::ViewToggleInfoPanelAction, input, ports);
    router.handleActionTriggered(ActionId::ViewToggleThumbnailPanelAction, input, ports);
    router.handleActionTriggered(ActionId::ViewGoToContentStartAction, input, ports);
    router.handleActionTriggered(ActionId::ViewGoToContentEndAction, input, ports);
    router.handleActionTriggered(ActionId::ViewScanForwardAction, input, ports);
    router.handleActionTriggered(ActionId::ViewScanBackwardAction, input, ports);
    router.handleActionTriggered(ActionId::WindowFullscreenAction, input, ports);
    router.handleActionTriggered(ActionId::HelpShortcutsAction, input, ports);
    router.handleActionTriggered(ActionId::OpenApplicationMenuAction, input, ports);
    router.handleActionTriggered(ActionId::ActionCount, input, ports);

    const QStringList expectedCalls {
        QStringLiteral("open-dialog"),
        QStringLiteral("open-with"),
        QStringLiteral("move-trash"),
        QStringLiteral("delete-permanent"),
        QStringLiteral("previous-container"),
        QStringLiteral("next-container"),
        QStringLiteral("previous-navigation"),
        QStringLiteral("next-navigation"),
        QStringLiteral("first-navigation"),
        QStringLiteral("last-navigation"),
        QStringLiteral("zoom-step:1"),
        QStringLiteral("zoom-step:-1"),
        QStringLiteral("manual-zoom:50"),
        QStringLiteral("manual-zoom:100"),
        QStringLiteral("manual-zoom:200"),
        QStringLiteral("fit"),
        QStringLiteral("fit-height"),
        QStringLiteral("fit-width"),
        QStringLiteral("rotate-clockwise"),
        QStringLiteral("rotate-counterclockwise"),
        QStringLiteral("toggle-two-page"),
        QStringLiteral("toggle-right-to-left"),
        QStringLiteral("toggle-info-panel"),
        QStringLiteral("toggle-thumbnail-panel"),
        QStringLiteral("pan-initial-scan-position"),
        QStringLiteral("pan-final-scan-position"),
        QStringLiteral("scan-forward"),
        QStringLiteral("next-navigation"),
        QStringLiteral("scan-backward"),
        QStringLiteral("previous-navigation"),
        QStringLiteral("toggle-fullscreen"),
        QStringLiteral("shortcut-help"),
        QStringLiteral("open-application-menu"),
    };
    QCOMPARE(log.actionCalls, expectedCalls);
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

void TestApplicationCommandRouter::videoPlaybackActionChecksModeAndVideoAvailability()
{
    ApplicationCommandRouter router;
    ApplicationCommandRouterInput input;
    CommandLog log;
    ApplicationCommandRouterPorts ports = commandPorts(log);

    router.handleActionTriggered(ActionId::ViewToggleVideoPlaybackAction, input, ports);
    QCOMPARE(log.toggleVideoPlaybackCount, 0);

    input.videoMode = true;
    log.videoAvailable = false;
    router.handleActionTriggered(ActionId::ViewToggleVideoPlaybackAction, input, ports);
    QCOMPARE(log.toggleVideoPlaybackCount, 0);

    log.videoAvailable = true;
    router.handleActionTriggered(ActionId::ViewToggleVideoPlaybackAction, input, ports);
    QCOMPARE(log.toggleVideoPlaybackCount, 1);
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

void TestApplicationCommandRouter::contentBoundaryActionsRouteByMediaMode()
{
    ApplicationCommandRouter router;
    ApplicationCommandRouterInput input;
    CommandLog log;
    ApplicationCommandRouterPorts ports = commandPorts(log);

    router.handleActionTriggered(ActionId::ViewGoToContentStartAction, input, ports);
    router.handleActionTriggered(ActionId::ViewGoToContentEndAction, input, ports);
    QCOMPARE(log.actionCalls,
        QStringList({ QStringLiteral("pan-initial-scan-position"),
            QStringLiteral("pan-final-scan-position") }));
    QCOMPARE(log.setVideoPositionCount, 0);

    input.videoMode = true;
    log = CommandLog {};
    ports = commandPorts(log);
    router.handleActionTriggered(ActionId::ViewGoToContentStartAction, input, ports);
    router.handleActionTriggered(ActionId::ViewGoToContentEndAction, input, ports);
    QCOMPARE(log.actionCalls, QStringList {});
    QCOMPARE(log.setVideoPositionCount, 2);
    QCOMPARE(log.lastVideoPosition, log.videoDuration);

    log = CommandLog {};
    log.videoDuration = 0;
    ports = commandPorts(log);
    router.handleActionTriggered(ActionId::ViewGoToContentEndAction, input, ports);
    QCOMPARE(log.setVideoPositionCount, 0);

    log = CommandLog {};
    log.videoSeekable = false;
    ports = commandPorts(log);
    router.handleActionTriggered(ActionId::ViewGoToContentStartAction, input, ports);
    router.handleActionTriggered(ActionId::ViewGoToContentEndAction, input, ports);
    QCOMPARE(log.setVideoPositionCount, 0);

    log = CommandLog {};
    log.videoAvailable = false;
    ports = commandPorts(log);
    router.handleActionTriggered(ActionId::ViewGoToContentStartAction, input, ports);
    router.handleActionTriggered(ActionId::ViewGoToContentEndAction, input, ports);
    QCOMPARE(log.setVideoPositionCount, 0);
}

void TestApplicationCommandRouter::zoomPresetDescriptorsOwnMetadataAndDispatch()
{
    ApplicationCommandRouter router;
    ApplicationCommandRouterInput input;
    kiriview::ApplicationActions::ApplicationActionStateInput stateInput;

    for (const kiriview::ApplicationActions::ZoomPresetDescriptor &descriptor :
        kiriview::ApplicationActions::zoomPresetDescriptors) {
        const kiriview::ApplicationActions::ActionDefinition *definition
            = kiriview::ApplicationActions::definitionForId(descriptor.actionId);
        QVERIFY(definition != nullptr);
        QCOMPARE(QString::fromLatin1(definition->name), QString::fromLatin1(descriptor.actionName));
        QCOMPARE(kiriview::ApplicationActions::localizedString(definition->text),
            kiriview::ApplicationActions::localizedString(descriptor.actionText));
        QCOMPARE(kiriview::ApplicationActions::applicationActionMenuText(
                     descriptor.actionId, stateInput),
            kiriview::ApplicationActions::localizedString(descriptor.menuText));

        CommandLog log;
        ApplicationCommandRouterPorts ports = commandPorts(log);
        router.handleActionTriggered(descriptor.actionId, input, ports);

        QCOMPARE(log.actionCalls,
            QStringList { QStringLiteral("manual-zoom:%1").arg(descriptor.zoomPercent) });
    }

    QVERIFY(kiriview::ApplicationActions::zoomPresetDescriptorForAction(
                ActionId::ViewZoom50PercentAction)
        != nullptr);
    QVERIFY(kiriview::ApplicationActions::zoomPresetDescriptorForAction(ActionId::ViewZoomInAction)
        == nullptr);
}

void TestApplicationCommandRouter::videoScanShortcutsUseActiveNavigation()
{
    ApplicationCommandRouter router;
    ApplicationCommandRouterInput input;
    input.videoMode = true;
    input.imagePannable = true;
    input.imageDocumentPageNavigationActive = true;
    input.atKnownFirstActiveNavigation = true;
    input.canOpenPreviousActiveNavigation = true;
    CommandLog log;
    log.scanMoved = true;
    ApplicationCommandRouterPorts ports = commandPorts(log);

    router.handleScanForwardAction(input, ports);
    router.handleScanBackwardAction(input, ports);

    QCOMPARE(log.scanForwardCount, 0);
    QCOMPARE(log.scanBackwardCount, 0);
    QCOMPARE(log.nextNavigationCount, 1);
    QCOMPARE(log.previousNavigationCount, 1);
    QCOMPARE(log.nextDisplayedImageStartToFinalScanPositionCount, 0);
    QCOMPARE(log.firstImageBoundaryCount, 0);
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
