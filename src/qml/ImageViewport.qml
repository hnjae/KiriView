// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQml
import io.github.hnjae.kiriview

Item {
    id: root

    property alias imageDocument: imageDocument
    property alias imageView: primaryImageView
    property alias flickable: imageFlickable
    property bool imageReady: imageDocument.status === KiriImageDocument.Ready
    property url initialSourceUrl
    readonly property int minimumManualZoomPercent: imageDocument.minimumManualZoomPercent
    readonly property int maximumManualZoomPercent: imageDocument.maximumManualZoomPercent
    readonly property int wheelAngleDeltaPerStep: 120
    readonly property bool imageHorizontallyPannable: imageFlickable.contentWidth > imageFlickable.width
    readonly property bool imagePannable: imageFlickable.contentWidth > imageFlickable.width || imageFlickable.contentHeight > imageFlickable.height
    readonly property real viewportWidth: imageFlickable.width
    readonly property real viewportHeight: imageFlickable.height

    signal viewerClicked

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

    function moveContentPosition(contentPosition) {
        const moved = contentPositionChanged(contentPosition);
        setContentPosition(contentPosition);
        return moved;
    }

    function setNextDisplayedImageStartToFinalScanPosition() {
        imageView.setNextDisplayedImageStartToFinalScanPosition();
    }

    function applyDisplayedImageInitialContentPosition() {
        setContentPosition(imageView.displayedImageInitialContentPosition());
    }

    function panBy(deltaX, deltaY) {
        if (!imagePannable) {
            return false;
        }

        const nextContentPosition = imageView.panContentPosition(currentContentPosition(), Qt.point(deltaX, deltaY));
        return moveContentPosition(nextContentPosition);
    }

    function panToBottomRight() {
        if (!imagePannable) {
            return false;
        }

        const nextContentPosition = imageView.finalScanContentPosition();
        return moveContentPosition(nextContentPosition);
    }

    function panToTopLeft() {
        if (!imagePannable) {
            return false;
        }

        return moveContentPosition(imageView.initialScanContentPosition());
    }

    function scanForward() {
        if (!imagePannable) {
            return false;
        }

        const nextContentPosition = imageView.nextScanContentPosition(currentContentPosition());
        return moveContentPosition(nextContentPosition);
    }

    function scanBackward() {
        if (!imagePannable) {
            return false;
        }

        const nextContentPosition = imageView.previousScanContentPosition(currentContentPosition());
        return moveContentPosition(nextContentPosition);
    }

    function viewportPointInsideImage(viewportX, viewportY) {
        if (!imageReady || imageView.width <= 0 || imageView.height <= 0) {
            return false;
        }

        return imageView.viewportPointInsideImage(currentContentPosition(), Qt.point(viewportX, viewportY));
    }

    function zoomByStep(stepCount, viewportX, viewportY) {
        if (!imageReady) {
            return false;
        }

        const nextZoomPercent = imageDocument.steppedManualZoomPercent(stepCount);
        if (Math.abs(nextZoomPercent - imageDocument.zoomPercent) < 0.001) {
            return false;
        }

        const nextContentPosition = imageView.zoomContentPosition(currentContentPosition(), Qt.point(viewportX, viewportY), nextZoomPercent);
        imageDocument.zoomPercent = nextZoomPercent;
        setContentPosition(nextContentPosition);
        return true;
    }

    function wheelZoomStepCount(wheel) {
        const verticalDelta = wheel.pixelDelta.y !== 0 ? wheel.pixelDelta.y : wheel.angleDelta.y;
        return verticalDelta / wheelAngleDeltaPerStep;
    }

    function wheelViewportPoint(wheel) {
        return Qt.point(wheel.x - imageFlickable.contentX, wheel.y - imageFlickable.contentY);
    }

    KiriImageDocument {
        id: imageDocument

        visibleItemRect: Qt.rect(imageFlickable.contentX - spreadItem.x, imageFlickable.contentY - spreadItem.y, imageFlickable.width, imageFlickable.height)
        viewportSize: Qt.size(imageFlickable.width, imageFlickable.height)

        Component.onCompleted: {
            if (root.initialSourceUrl.toString().length > 0) {
                sourceUrl = root.initialSourceUrl;
            }
        }
    }

    Flickable {
        id: imageFlickable

        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        contentHeight: Math.max(height, spreadItem.y + spreadItem.height)
        contentWidth: Math.max(width, spreadItem.x + spreadItem.width)
        interactive: imageDocument.status === KiriImageDocument.Ready && (contentWidth > width || contentHeight > height)

        Controls.ScrollBar.horizontal: Controls.ScrollBar {
            policy: imageFlickable.contentWidth > imageFlickable.width ? Controls.ScrollBar.AsNeeded : Controls.ScrollBar.AlwaysOff
        }

        Controls.ScrollBar.vertical: Controls.ScrollBar {
            policy: imageFlickable.contentHeight > imageFlickable.height ? Controls.ScrollBar.AsNeeded : Controls.ScrollBar.AlwaysOff
        }

        WheelHandler {
            id: zoomWheelHandler

            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            acceptedModifiers: Qt.ControlModifier
            blocking: true
            enabled: root.imageReady
            target: null

            onWheel: wheel => {
                const stepCount = root.wheelZoomStepCount(wheel);
                const viewportPoint = root.wheelViewportPoint(wheel);
                const insideImage = root.viewportPointInsideImage(viewportPoint.x, viewportPoint.y);
                console.debug(inputLog, "ctrl-wheel received", "rawX", wheel.x, "rawY", wheel.y, "viewportX", viewportPoint.x, "viewportY", viewportPoint.y, "pixelDelta", wheel.pixelDelta, "angleDelta", wheel.angleDelta, "stepCount", stepCount, "insideImage", insideImage, "zoomPercent", imageDocument.zoomPercent, "contentX", imageFlickable.contentX, "contentY", imageFlickable.contentY, "contentWidth", imageFlickable.contentWidth, "contentHeight", imageFlickable.contentHeight, "viewportWidth", imageFlickable.width, "viewportHeight", imageFlickable.height);

                if (stepCount === 0 || !insideImage) {
                    console.debug(inputLog, "ctrl-wheel ignored", "reason", stepCount === 0 ? "zero-step" : "outside-image", "rawX", wheel.x, "rawY", wheel.y, "viewportX", viewportPoint.x, "viewportY", viewportPoint.y);
                    wheel.accepted = false;
                    return;
                }

                const previousZoomPercent = imageDocument.zoomPercent;
                const previousContentX = imageFlickable.contentX;
                const previousContentY = imageFlickable.contentY;
                const zoomed = root.zoomByStep(stepCount, viewportPoint.x, viewportPoint.y);
                console.debug(inputLog, "ctrl-wheel zoomed", "applied", zoomed, "previousZoomPercent", previousZoomPercent, "nextZoomPercent", imageDocument.zoomPercent, "previousContentX", previousContentX, "previousContentY", previousContentY, "nextContentX", imageFlickable.contentX, "nextContentY", imageFlickable.contentY);
                wheel.accepted = true;
            }
        }

        Item {
            id: spreadItem

            height: imageDocument.displaySize.height
            width: imageDocument.displaySize.width
            x: Math.max(0, (imageFlickable.width - width) / 2)
            y: Math.max(0, (imageFlickable.height - height) / 2)

            KiriImageView {
                id: primaryImageView

                document: imageDocument
                height: imageDocument.primaryDisplaySize.height
                width: imageDocument.primaryDisplaySize.width
                x: imageDocument.secondaryPageVisible && imageDocument.rightToLeftReadingEnabled && imageDocument.rightToLeftReadingAvailable ? secondaryImageView.width : 0
                y: Math.max(0, (spreadItem.height - height) / 2)

                onDisplayedImageInitialContentPositionRequested: Qt.callLater(root.applyDisplayedImageInitialContentPosition)
            }

            KiriImageView {
                id: secondaryImageView

                document: imageDocument
                height: imageDocument.secondaryDisplaySize.height
                secondaryPage: true
                visible: imageDocument.secondaryPageVisible
                width: imageDocument.secondaryDisplaySize.width
                x: imageDocument.rightToLeftReadingEnabled && imageDocument.rightToLeftReadingAvailable ? 0 : primaryImageView.width
                y: Math.max(0, (spreadItem.height - height) / 2)
            }
        }
    }

    TapHandler {
        acceptedButtons: Qt.LeftButton

        onTapped: root.viewerClicked()
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
