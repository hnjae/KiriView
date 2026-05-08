// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import io.github.hnjae.kiriview

Item {
    id: root

    property alias imageDocument: imageDocument
    property alias imageView: primaryImageView
    property alias flickable: imageFlickable
    property bool imageReady: imageDocument.status === KiriImageDocument.Ready
    property bool pendingFinalScanStart: false
    property bool displayedImageUsesFinalScanStart: false
    property url initialSourceUrl
    readonly property int minimumManualZoomPercent: imageDocument.minimumManualZoomPercent
    readonly property int maximumManualZoomPercent: imageDocument.maximumManualZoomPercent
    readonly property int zoomStepPercent: imageDocument.zoomStepPercent
    readonly property int wheelAngleDeltaPerStep: 120
    readonly property bool imageHorizontallyPannable: imageFlickable.contentWidth > imageFlickable.width
    readonly property bool imagePannable: imageFlickable.contentWidth > imageFlickable.width || imageFlickable.contentHeight > imageFlickable.height
    readonly property real viewportWidth: imageFlickable.width
    readonly property real viewportHeight: imageFlickable.height

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

    function resetContentPositionToTopLeft() {
        setContentPosition(Qt.point(0, 0));
    }

    function setNextDisplayedImageStartToFinalScanPosition() {
        pendingFinalScanStart = true;
    }

    function applyDisplayedImageInitialContentPosition() {
        if (displayedImageUsesFinalScanStart) {
            setContentPosition(imageView.finalScanContentPosition());
            displayedImageUsesFinalScanStart = false;
            return;
        }

        resetContentPositionToTopLeft();
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

        return moveContentPosition(Qt.point(0, 0));
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

    function zoomBy(deltaPercent, viewportX, viewportY) {
        if (!imageReady) {
            return false;
        }

        const nextZoomPercent = imageDocument.clampedManualZoomPercent(imageDocument.zoomPercent + deltaPercent);
        if (Math.abs(nextZoomPercent - imageDocument.zoomPercent) < 0.001) {
            return false;
        }

        const nextContentPosition = imageView.zoomContentPosition(currentContentPosition(), Qt.point(viewportX, viewportY), nextZoomPercent);
        imageDocument.zoomPercent = nextZoomPercent;
        setContentPosition(nextContentPosition);
        return true;
    }

    function wheelZoomDeltaPercent(wheel) {
        const verticalDelta = wheel.pixelDelta.y !== 0 ? wheel.pixelDelta.y : wheel.angleDelta.y;
        return verticalDelta / wheelAngleDeltaPerStep * zoomStepPercent;
    }

    KiriImageDocument {
        id: imageDocument

        visibleItemRect: Qt.rect(imageFlickable.contentX - spreadItem.x, imageFlickable.contentY - spreadItem.y, imageFlickable.width, imageFlickable.height)
        viewportSize: Qt.size(imageFlickable.width, imageFlickable.height)

        onDisplayedUrlChanged: {
            root.displayedImageUsesFinalScanStart = root.pendingFinalScanStart;
            root.pendingFinalScanStart = false;
            Qt.callLater(root.applyDisplayedImageInitialContentPosition);
        }
        onLoadingChanged: {
            if (!loading) {
                root.pendingFinalScanStart = false;
            }
        }

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
                x: 0
                y: Math.max(0, (spreadItem.height - height) / 2)
            }

            KiriImageView {
                id: secondaryImageView

                document: imageDocument
                height: imageDocument.secondaryDisplaySize.height
                secondaryPage: true
                visible: imageDocument.secondaryPageVisible
                width: imageDocument.secondaryDisplaySize.width
                x: primaryImageView.width
                y: Math.max(0, (spreadItem.height - height) / 2)
            }
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

    WheelHandler {
        id: zoomWheelHandler

        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        acceptedModifiers: Qt.ControlModifier
        blocking: true
        enabled: root.imageReady
        target: null

        onWheel: wheel => {
            const deltaPercent = root.wheelZoomDeltaPercent(wheel);
            if (deltaPercent === 0 || !root.viewportPointInsideImage(wheel.x, wheel.y)) {
                wheel.accepted = false;
                return;
            }

            root.zoomBy(deltaPercent, wheel.x, wheel.y);
            wheel.accepted = true;
        }
    }

    WheelHandler {
        id: horizontalPanWheelHandler

        acceptedModifiers: Qt.ShiftModifier
        blocking: true
        enabled: root.imageReady && root.imageHorizontallyPannable
        target: null

        onWheel: wheel => {
            const verticalDelta = wheel.pixelDelta.y !== 0 ? wheel.pixelDelta.y : wheel.angleDelta.y;
            if (verticalDelta === 0) {
                wheel.accepted = false;
                return;
            }

            root.panBy(-verticalDelta, 0);
            wheel.accepted = true;
        }
    }
}
