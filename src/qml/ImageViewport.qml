// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

Item {
    id: root

    property alias imageDocument: imageDocument
    property alias imageView: imageView
    property alias flickable: imageFlickable
    property bool imageReady: imageDocument.status === KiriImageDocument.Ready
    property bool pendingFinalScanStart: false
    property bool displayedImageUsesFinalScanStart: false
    property url initialSourceUrl
    readonly property int minimumManualZoomPercent: imageDocument.minimumManualZoomPercent
    readonly property int maximumManualZoomPercent: imageDocument.maximumManualZoomPercent
    readonly property int zoomStepPercent: imageDocument.zoomStepPercent
    readonly property real dragZoomPercentPerPixel: zoomStepPercent / Math.max(1, Kirigami.Units.gridUnit * 2)
    readonly property bool imageHorizontallyPannable: imageFlickable.contentWidth > imageFlickable.width
    readonly property bool imagePannable: imageFlickable.contentWidth > imageFlickable.width || imageFlickable.contentHeight > imageFlickable.height
    readonly property real viewportWidth: imageFlickable.width
    readonly property real viewportHeight: imageFlickable.height

    function resetContentPositionToTopLeft() {
        imageFlickable.contentX = 0;
        imageFlickable.contentY = 0;
    }

    function setContentPosition(contentPosition) {
        imageFlickable.contentX = contentPosition.x;
        imageFlickable.contentY = contentPosition.y;
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

        const currentContentPosition = Qt.point(imageFlickable.contentX, imageFlickable.contentY);
        const nextContentPosition = imageView.panContentPosition(currentContentPosition, Qt.point(deltaX, deltaY));
        const moved = nextContentPosition.x !== imageFlickable.contentX || nextContentPosition.y !== imageFlickable.contentY;
        setContentPosition(nextContentPosition);
        return moved;
    }

    function scanForward() {
        if (!imagePannable) {
            return false;
        }

        const currentContentPosition = Qt.point(imageFlickable.contentX, imageFlickable.contentY);
        const nextContentPosition = imageView.nextScanContentPosition(currentContentPosition);
        const moved = nextContentPosition.x !== imageFlickable.contentX || nextContentPosition.y !== imageFlickable.contentY;
        setContentPosition(nextContentPosition);
        return moved;
    }

    function scanBackward() {
        if (!imagePannable) {
            return false;
        }

        const currentContentPosition = Qt.point(imageFlickable.contentX, imageFlickable.contentY);
        const nextContentPosition = imageView.previousScanContentPosition(currentContentPosition);
        const moved = nextContentPosition.x !== imageFlickable.contentX || nextContentPosition.y !== imageFlickable.contentY;
        setContentPosition(nextContentPosition);
        return moved;
    }

    function viewportPointInsideImage(viewportX, viewportY) {
        if (!imageReady || imageView.width <= 0 || imageView.height <= 0) {
            return false;
        }

        return imageView.viewportPointInsideImage(Qt.point(imageFlickable.contentX, imageFlickable.contentY), Qt.point(viewportX, viewportY));
    }

    function zoomBy(deltaPercent, viewportX, viewportY) {
        if (!imageReady) {
            return false;
        }

        const nextZoomPercent = imageDocument.clampedManualZoomPercent(imageDocument.zoomPercent + deltaPercent);
        if (Math.abs(nextZoomPercent - imageDocument.zoomPercent) < 0.001) {
            return false;
        }

        const nextContentPosition = imageView.zoomContentPosition(Qt.point(imageFlickable.contentX, imageFlickable.contentY), Qt.point(viewportX, viewportY), nextZoomPercent);
        imageDocument.zoomPercent = nextZoomPercent;
        setContentPosition(nextContentPosition);
        return true;
    }

    KiriImageDocument {
        id: imageDocument

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
        contentHeight: Math.max(height, imageView.y + imageView.height)
        contentWidth: Math.max(width, imageView.x + imageView.width)
        interactive: imageDocument.status === KiriImageDocument.Ready && (contentWidth > width || contentHeight > height)

        Controls.ScrollBar.horizontal: Controls.ScrollBar {
            policy: imageFlickable.contentWidth > imageFlickable.width ? Controls.ScrollBar.AsNeeded : Controls.ScrollBar.AlwaysOff
        }

        Controls.ScrollBar.vertical: Controls.ScrollBar {
            policy: imageFlickable.contentHeight > imageFlickable.height ? Controls.ScrollBar.AsNeeded : Controls.ScrollBar.AlwaysOff
        }

        KiriImageView {
            id: imageView

            document: imageDocument
            height: imageDocument.displaySize.height
            width: imageDocument.displaySize.width
            x: Math.max(0, (imageFlickable.width - width) / 2)
            y: Math.max(0, (imageFlickable.height - height) / 2)
        }
    }

    MouseArea {
        id: zoomDragArea

        anchors.fill: imageFlickable
        acceptedButtons: Qt.LeftButton
        cursorShape: root.imagePannable ? (dragCursorActive ? Qt.ClosedHandCursor : Qt.OpenHandCursor) : Qt.ArrowCursor
        enabled: root.imageReady

        property bool dragCursorActive: imageFlickable.draggingHorizontally || imageFlickable.draggingVertically || zoomDragActive
        property real lastY: 0
        property bool zoomDragActive: false

        onCanceled: zoomDragActive = false
        onWheel: wheel => {
            if (!(wheel.modifiers & Qt.ShiftModifier) || !root.imageHorizontallyPannable) {
                wheel.accepted = false;
                return;
            }

            const verticalDelta = wheel.pixelDelta.y !== 0 ? wheel.pixelDelta.y : wheel.angleDelta.y;
            if (verticalDelta === 0) {
                wheel.accepted = false;
                return;
            }

            root.panBy(-verticalDelta, 0);
            wheel.accepted = true;
        }
        onPositionChanged: mouse => {
            if (!zoomDragActive) {
                mouse.accepted = false;
                return;
            }

            const deltaY = mouse.y - lastY;
            if (deltaY !== 0) {
                root.zoomBy(-deltaY * root.dragZoomPercentPerPixel, mouse.x, mouse.y);
                lastY = mouse.y;
            }
            mouse.accepted = true;
        }
        onPressed: mouse => {
            if ((mouse.modifiers & Qt.ControlModifier) && root.viewportPointInsideImage(mouse.x, mouse.y)) {
                zoomDragActive = true;
                lastY = mouse.y;
                mouse.accepted = true;
                return;
            }

            zoomDragActive = false;
            mouse.accepted = false;
        }
        onReleased: mouse => {
            zoomDragActive = false;
            mouse.accepted = true;
        }
    }
}
