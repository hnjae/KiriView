// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQml
import io.github.hnjae.kiriview

MediaViewportDelegate {
    id: root

    property alias imageView: primaryImageView
    property alias flickable: imageFlickable
    property bool applyingViewportProjection: false
    property int appliedViewportCommandRevision: 0
    property int appliedViewportObservationRevision: 0
    readonly property var imageDocument: root.documentSession.imageDocument
    property bool imageReady: root.presentationActive && root.documentSession.activeImageReady
    readonly property int minimumManualZoomPercent: root.imageDocument.minimumManualZoomPercent
    readonly property int maximumManualZoomPercent: root.imageDocument.maximumManualZoomPercent
    readonly property int wheelAngleDeltaPerStep: 120
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

        function scanBackward() {
            return root.scanBackward();
        }

        function scanForward() {
            return root.scanForward();
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

        name: "io.github.hnjae.kiriview.input"
        defaultLogLevel: LoggingCategory.Warning
    }

    function currentContentPosition() {
        return Qt.point(imageFlickable.contentX, imageFlickable.contentY);
    }

    function contentPositionChanged(contentPosition) {
        return contentPosition.x !== imageFlickable.contentX || contentPosition.y !== imageFlickable.contentY;
    }

    function setContentPosition(contentPosition) {
        imageFlickable.contentX = contentPosition.x;
        imageFlickable.contentY = contentPosition.y;
    }

    function applyPhysicalContentPosition(contentPosition) {
        const moved = contentPositionChanged(contentPosition);
        setContentPosition(contentPosition);
        return moved;
    }

    function moveContentPosition(contentPosition) {
        if (!root.presentationActive || root.imageDocument === null) {
            return false;
        }

        const previousContentPosition = currentContentPosition();
        const commandRevision = root.imageDocument.requestViewportContentPosition(contentPosition);
        root.applyViewportProjection();
        return previousContentPosition.x !== imageFlickable.contentX || previousContentPosition.y !== imageFlickable.contentY || commandRevision > root.appliedViewportCommandRevision;
    }

    function applyViewportProjection() {
        if (!root.presentationActive || root.imageDocument === null || root.applyingViewportProjection) {
            return;
        }

        const commandRevision = root.imageDocument.viewportCommandRevision;
        const observationRevision = root.imageDocument.viewportObservationRevision;
        if (root.imageDocument.viewportCommandStatus === KiriImageDocument.Rejected) {
            return;
        }
        if (commandRevision < root.appliedViewportCommandRevision) {
            return;
        }
        if (commandRevision === root.appliedViewportCommandRevision && observationRevision <= root.appliedViewportObservationRevision) {
            return;
        }

        root.applyingViewportProjection = true;
        if (commandRevision > root.appliedViewportCommandRevision) {
            root.imageDocument.beginViewportCommandApplication(commandRevision);
        }
        applyPhysicalContentPosition(root.imageDocument.viewportContentPosition);
        if (commandRevision > root.appliedViewportCommandRevision) {
            root.imageDocument.completeViewportCommandApplication(commandRevision, currentContentPosition());
            root.appliedViewportCommandRevision = commandRevision;
            root.imageDocument.acknowledgeViewportCommand(commandRevision, currentContentPosition());
        }
        root.appliedViewportObservationRevision = Math.max(root.appliedViewportObservationRevision, observationRevision);
        root.applyingViewportProjection = false;
    }

    function observeViewportContentPosition(origin) {
        if (!root.presentationActive || root.imageDocument === null || root.applyingViewportProjection) {
            return false;
        }

        return root.imageDocument.observeViewportContentPosition(currentContentPosition(), origin);
    }

    function setNextDisplayedImageStartToFinalScanPosition() {
        root.imageDocument.requestNextDisplayedImageStartToFinalScanPosition();
    }

    function applyDisplayedImageInitialContentPosition() {
        root.imageDocument.requestDisplayedImageInitialContentPosition();
        root.applyViewportProjection();
    }

    function panBy(deltaX, deltaY) {
        const moved = root.imageDocument.requestViewportPanBy(deltaX, deltaY);
        root.applyViewportProjection();
        return moved;
    }

    function panToBottomRight() {
        const moved = root.imageDocument.requestViewportPanToFinalScanPosition();
        root.applyViewportProjection();
        return moved;
    }

    function panToTopLeft() {
        const moved = root.imageDocument.requestViewportPanToInitialScanPosition();
        root.applyViewportProjection();
        return moved;
    }

    function scanForward() {
        const moved = root.imageDocument.requestViewportScanForward();
        root.applyViewportProjection();
        return moved;
    }

    function scanBackward() {
        const moved = root.imageDocument.requestViewportScanBackward();
        root.applyViewportProjection();
        return moved;
    }

    function viewportPointInsideImage(viewportX, viewportY) {
        if (!imageReady || imageView.width <= 0 || imageView.height <= 0) {
            return false;
        }

        return imageView.viewportPointInsideImage(currentContentPosition(), Qt.point(viewportX, viewportY));
    }

    function nearestImageViewportPoint(viewportX, viewportY) {
        if (!imageReady || imageView.width <= 0 || imageView.height <= 0) {
            return null;
        }

        const point = imageView.nearestImageViewportPoint(currentContentPosition(), Qt.point(viewportX, viewportY));
        return Number.isFinite(point.x) && Number.isFinite(point.y) ? point : null;
    }

    function zoomByStep(stepCount, viewportX, viewportY) {
        if (!imageReady) {
            return false;
        }

        const zoomed = root.imageDocument.requestZoomByStep(stepCount, Qt.point(viewportX, viewportY));
        root.applyViewportProjection();
        return zoomed;
    }

    function toggleFitOrActualSize(viewportX, viewportY) {
        if (!imageReady) {
            return false;
        }

        const toggled = root.imageDocument.requestToggleFitOrActualSize(Qt.point(viewportX, viewportY));
        root.applyViewportProjection();
        return toggled;
    }

    function wheelZoomStepCount(wheel) {
        const verticalDelta = wheel.pixelDelta.y !== 0 ? wheel.pixelDelta.y : wheel.angleDelta.y;
        return verticalDelta / wheelAngleDeltaPerStep;
    }

    function wheelViewportPoint(wheel) {
        return Qt.point(wheel.x - imageFlickable.contentX, wheel.y - imageFlickable.contentY);
    }

    function handleWheelZoom(wheel, inputGesture) {
        const stepCount = root.wheelZoomStepCount(wheel);
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

    Binding {
        target: root.imageDocument
        property: "viewportSize"
        value: Qt.size(imageFlickable.width, imageFlickable.height)
        when: root.presentationActive && root.imageDocument !== null
    }

    Connections {
        target: root.presentationActive ? root.imageDocument : null

        function onViewportFrameChanged() {
            root.applyViewportProjection();
        }
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

                if (root.handleWheelZoom(wheel, "right-button")) {
                    root.markContextMenuTapSuppressed();
                }
            }
        }

        Item {
            id: spreadItem

            height: root.presentationActive ? root.imageDocument.displaySize.height : 0
            width: root.presentationActive ? root.imageDocument.displaySize.width : 0
            x: root.presentationActive ? root.imageDocument.viewportImageRect.x : 0
            y: root.presentationActive ? root.imageDocument.viewportImageRect.y : 0

            KiriImageView {
                id: primaryImageView

                document: root.presentationActive ? root.imageDocument : null
                height: root.presentationActive ? root.imageDocument.primaryDisplaySize.height : 0
                width: root.presentationActive ? root.imageDocument.primaryDisplaySize.width : 0
                x: root.presentationActive && root.imageDocument.secondaryPageVisible && root.documentSession.activeImageRightToLeftReadingActive ? secondaryImageView.width : 0
                y: Math.max(0, (spreadItem.height - height) / 2)

                onDisplayedImageInitialContentPositionRequested: root.applyDisplayedImageInitialContentPosition()
            }

            KiriImageView {
                id: secondaryImageView

                document: root.presentationActive ? root.imageDocument : null
                height: root.presentationActive ? root.imageDocument.secondaryDisplaySize.height : 0
                secondaryPage: true
                visible: root.presentationActive && root.imageDocument.secondaryPageVisible
                width: root.presentationActive ? root.imageDocument.secondaryDisplaySize.width : 0
                x: root.presentationActive && root.documentSession.activeImageRightToLeftReadingActive ? 0 : primaryImageView.width
                y: Math.max(0, (spreadItem.height - height) / 2)
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
