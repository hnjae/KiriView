// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQml
import org.hnjae.kiriview

MediaViewportDelegate {
    id: root

    property alias imageView: primaryDisplayImagePage
    property alias flickable: imageFlickable
    readonly property var imageDocument: root.documentSession.imageDocument
    property bool imageReady: root.presentationActive && root.documentSession.activeImageReady
    readonly property int minimumManualZoomPercent: root.imageDocument.minimumManualZoomPercent
    readonly property int maximumManualZoomPercent: root.imageDocument.maximumManualZoomPercent
    readonly property bool imageHorizontallyPannable: root.presentationActive && root.imageDocument.viewportHorizontallyPannable
    readonly property bool imagePannable: root.presentationActive && root.imageDocument.viewportPannable
    readonly property real viewportWidth: imageFlickable.width
    readonly property real viewportHeight: imageFlickable.height

    imageInteractionSurface: ImageViewportInteractionSurface {
        imageHorizontallyPannable: root.imageHorizontallyPannable
        imagePannable: root.imagePannable
        viewportHeight: root.viewportHeight
        viewportWidth: root.viewportWidth

        function panBy(deltaX, deltaY) {
            return root.panBy(deltaX, deltaY);
        }

        function panToBottomRight() {
            return root.panToBottomRight();
        }

        function panToTopLeft() {
            return root.panToTopLeft();
        }

        function setNextDisplayedImageStartToFinalScanPosition() {
            root.setNextDisplayedImageStartToFinalScanPosition();
        }

        function zoomByStep(stepCount, viewportX, viewportY) {
            return root.zoomByStep(stepCount, viewportX, viewportY);
        }
    }

    LoggingCategory {
        id: inputLog

        name: "org.hnjae.kiriview.input"
        defaultLogLevel: LoggingCategory.Warning
    }

    function moveContentPosition(contentPosition) {
        return viewportCommandBridge.requestContentPosition(contentPosition);
    }

    function observeViewportContentPosition(origin) {
        return viewportCommandBridge.observeViewportContentPosition(origin);
    }

    function setNextDisplayedImageStartToFinalScanPosition() {
        root.imageDocument.requestNextDisplayedImageStartToFinalScanPosition();
    }

    function applyDisplayedImageInitialContentPosition() {
        root.imageDocument.requestDisplayedImageInitialContentPosition();
    }

    function panBy(deltaX, deltaY) {
        const moved = root.imageDocument.requestViewportPanBy(deltaX, deltaY);
        return moved;
    }

    function panToBottomRight() {
        const moved = root.imageDocument.requestViewportPanToFinalScanPosition();
        return moved;
    }

    function panToTopLeft() {
        const moved = root.imageDocument.requestViewportPanToInitialScanPosition();
        return moved;
    }

    function viewportPointInsideImage(viewportX, viewportY) {
        if (!imageReady || imageView.width <= 0 || imageView.height <= 0) {
            return false;
        }

        return root.imageDocument.viewportPointInsideImage(Qt.point(viewportX, viewportY));
    }

    function nearestImageViewportPoint(viewportX, viewportY) {
        if (!imageReady || imageView.width <= 0 || imageView.height <= 0) {
            return null;
        }

        const point = root.imageDocument.nearestImageViewportPoint(Qt.point(viewportX, viewportY));
        return Number.isFinite(point.x) && Number.isFinite(point.y) ? point : null;
    }

    function zoomByStep(stepCount, viewportX, viewportY) {
        if (!imageReady) {
            return false;
        }

        const zoomed = root.imageDocument.requestZoomByStep(stepCount, Qt.point(viewportX, viewportY));
        return zoomed;
    }

    function toggleFitOrActualSize(viewportX, viewportY) {
        if (!imageReady) {
            return false;
        }

        const toggled = root.imageDocument.requestToggleFitOrActualSize(Qt.point(viewportX, viewportY));
        return toggled;
    }

    function wheelViewportPoint(wheel) {
        return Qt.point(wheel.x - imageFlickable.contentX, wheel.y - imageFlickable.contentY);
    }

    function handleWheelZoom(wheel, inputGesture) {
        const stepCount = wheelZoomPolicy.stepCount(wheel);
        const viewportPoint = root.wheelViewportPoint(wheel);
        const insideImage = root.viewportPointInsideImage(viewportPoint.x, viewportPoint.y);
        const anchorPoint = root.nearestImageViewportPoint(viewportPoint.x, viewportPoint.y);
        console.debug(inputLog, "wheel zoom received", "gesture", inputGesture, "rawX", wheel.x, "rawY", wheel.y, "viewportX", viewportPoint.x, "viewportY", viewportPoint.y, "anchorX", anchorPoint === null ? NaN : anchorPoint.x, "anchorY", anchorPoint === null ? NaN : anchorPoint.y, "pixelDelta", wheel.pixelDelta, "angleDelta", wheel.angleDelta, "stepCount", stepCount, "insideImage", insideImage, "zoomPercent", root.imageDocument.zoomPercent, "contentX", imageFlickable.contentX, "contentY", imageFlickable.contentY, "contentWidth", imageFlickable.contentWidth, "contentHeight", imageFlickable.contentHeight, "viewportWidth", imageFlickable.width, "viewportHeight", imageFlickable.height);

        if (stepCount === 0 || anchorPoint === null) {
            console.debug(inputLog, "wheel zoom ignored", "gesture", inputGesture, "reason", stepCount === 0 ? "zero-step" : "missing-anchor", "rawX", wheel.x, "rawY", wheel.y, "viewportX", viewportPoint.x, "viewportY", viewportPoint.y);
            wheel.accepted = false;
            return false;
        }

        const previousZoomPercent = root.imageDocument.zoomPercent;
        const previousContentX = imageFlickable.contentX;
        const previousContentY = imageFlickable.contentY;
        const zoomed = root.zoomByStep(stepCount, anchorPoint.x, anchorPoint.y);
        console.debug(inputLog, "wheel zoom applied", "gesture", inputGesture, "applied", zoomed, "previousZoomPercent", previousZoomPercent, "nextZoomPercent", root.imageDocument.zoomPercent, "previousContentX", previousContentX, "previousContentY", previousContentY, "nextContentX", imageFlickable.contentX, "nextContentY", imageFlickable.contentY);
        wheel.accepted = true;
        return true;
    }

    ZoomWheelStepPolicy {
        id: wheelZoomPolicy
    }

    Binding {
        target: root.imageDocument
        property: "viewportSize"
        value: Qt.size(imageFlickable.width, imageFlickable.height)
        when: root.presentationActive && root.imageDocument !== null
    }

    Connections {
        target: root.presentationActive ? root.imageDocument : null

        function onDisplayedUrlChanged() {
            root.applyDisplayedImageInitialContentPosition();
        }
    }

    KiriImageViewportCommandBridge {
        id: viewportCommandBridge

        active: root.presentationActive
        document: root.presentationActive ? root.imageDocument : null
        target: imageFlickable
    }

    Flickable {
        id: imageFlickable

        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        contentHeight: root.presentationActive ? root.imageDocument.viewportContentSize.height : height
        contentWidth: root.presentationActive ? root.imageDocument.viewportContentSize.width : width
        interactive: root.imageReady && root.imagePannable

        onMovementEnded: root.observeViewportContentPosition(KiriImageDocument.Inertia)

        Controls.ScrollBar.horizontal: Controls.ScrollBar {
            policy: root.imageHorizontallyPannable ? Controls.ScrollBar.AsNeeded : Controls.ScrollBar.AlwaysOff
        }

        Controls.ScrollBar.vertical: Controls.ScrollBar {
            policy: root.presentationActive && root.imageDocument.viewportVerticallyPannable ? Controls.ScrollBar.AsNeeded : Controls.ScrollBar.AlwaysOff
        }

        WheelHandler {
            id: ctrlZoomWheelHandler

            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            acceptedModifiers: Qt.ControlModifier
            blocking: true
            enabled: root.imageReady
            target: null

            onWheel: wheel => {
                root.handleWheelZoom(wheel, "ctrl");
            }
        }

        WheelHandler {
            id: rightButtonZoomWheelHandler

            acceptedButtons: Qt.RightButton
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            acceptedModifiers: Qt.NoModifier
            blocking: true
            enabled: root.imageReady && root.contextMenuButtonPressed
            target: null

            onWheel: wheel => {
                if ((wheel.buttons & Qt.RightButton) === 0) {
                    wheel.accepted = false;
                    return;
                }

                root.markContextMenuTapSuppressed();
                root.handleWheelZoom(wheel, "right-button");
            }
        }

        Item {
            id: spreadItem

            height: root.imageReady ? root.imageDocument.displayHeight : 0
            width: root.imageReady ? root.imageDocument.displayWidth : 0
            x: root.imageReady ? root.imageDocument.viewportImageRect.x : 0
            y: root.imageReady ? root.imageDocument.viewportImageRect.y : 0

            DisplayImagePage {
                id: primaryDisplayImagePage

                displaySource: root.presentationActive ? root.imageDocument.primaryDisplaySource : null
                height: root.imageReady ? root.imageDocument.primaryDisplayHeight : 0
                objectName: "primaryDisplayImagePage"
                width: root.imageReady ? root.imageDocument.primaryDisplayWidth : 0
                x: root.presentationActive && root.imageDocument.secondaryPageVisible && root.documentSession.activeImageRightToLeftReadingActive ? secondaryDisplayImagePage.width : 0
                y: Math.max(0, (spreadItem.height - height) / 2)

                onLoadOutcomeAcknowledged: (providerUrl, revisionToken, sourceIdentity, outcome) => {
                    root.imageDocument.acknowledgeDisplayImageLoad(displaySource.pageRole, providerUrl, revisionToken, sourceIdentity, outcome);
                }
            }

            DisplayImagePage {
                id: secondaryDisplayImagePage

                displaySource: root.presentationActive ? root.imageDocument.secondaryDisplaySource : null
                height: root.imageReady ? root.imageDocument.secondaryDisplayHeight : 0
                objectName: "secondaryDisplayImagePage"
                width: root.imageReady ? root.imageDocument.secondaryDisplayWidth : 0
                x: root.presentationActive && root.documentSession.activeImageRightToLeftReadingActive ? 0 : primaryDisplayImagePage.width
                y: Math.max(0, (spreadItem.height - height) / 2)

                onLoadOutcomeAcknowledged: (providerUrl, revisionToken, sourceIdentity, outcome) => {
                    root.imageDocument.acknowledgeDisplayImageLoad(displaySource.pageRole, providerUrl, revisionToken, sourceIdentity, outcome);
                }
            }

            KiriImageViewportContextBridge {
                id: primaryContextBridge

                document: root.presentationActive ? root.imageDocument : null
                height: primaryDisplayImagePage.height
                objectName: "primaryContextBridge"
                width: primaryDisplayImagePage.width
                x: primaryDisplayImagePage.x
                y: primaryDisplayImagePage.y
            }

            KiriImageViewportContextBridge {
                id: secondaryContextBridge

                document: root.presentationActive ? root.imageDocument : null
                height: secondaryDisplayImagePage.height
                objectName: "secondaryContextBridge"
                secondaryPage: true
                visible: secondaryDisplayImagePage.visible
                width: secondaryDisplayImagePage.width
                x: secondaryDisplayImagePage.x
                y: secondaryDisplayImagePage.y
            }
        }
    }

    Timer {
        id: singleClickTimer

        // qmllint disable missing-property
        interval: Qt.application.styleHints.mouseDoubleClickInterval
        // qmllint enable missing-property
        repeat: false

        onTriggered: root.viewerClicked()
    }

    MouseArea {
        id: leftClickMouseArea

        property bool suppressNextSingleClick: false

        acceptedButtons: Qt.LeftButton
        anchors.fill: parent
        preventStealing: false

        onClicked: mouse => {
            if (leftClickMouseArea.suppressNextSingleClick) {
                leftClickMouseArea.suppressNextSingleClick = false;
                mouse.accepted = true;
                return;
            }

            singleClickTimer.restart();
        }
        onDoubleClicked: mouse => {
            leftClickMouseArea.suppressNextSingleClick = true;
            singleClickTimer.stop();
            root.toggleFitOrActualSize(mouse.x, mouse.y);
        }
    }

    HoverHandler {
        id: imageHoverHandler

        cursorShape: {
            if (!root.imagePannable) {
                return Qt.ArrowCursor;
            }

            if (imageFlickable.draggingHorizontally || imageFlickable.draggingVertically) {
                return Qt.ClosedHandCursor;
            }

            return Qt.OpenHandCursor;
        }
        enabled: root.imageReady
    }
}
