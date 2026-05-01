// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

Item {
    id: root

    property alias imageView: imageView
    property alias flickable: imageFlickable
    property bool imageReady: imageView.status === KiriImageView.Ready
    property url initialSourceUrl
    property int minimumManualZoomPercent: 10
    property int maximumManualZoomPercent: 800
    property int zoomStepPercent: 10
    readonly property real dragZoomPercentPerPixel: zoomStepPercent / Math.max(1, Kirigami.Units.gridUnit * 2)
    readonly property bool imagePannable: imageFlickable.contentWidth > imageFlickable.width || imageFlickable.contentHeight > imageFlickable.height
    readonly property real viewportWidth: imageFlickable.width
    readonly property real viewportHeight: imageFlickable.height

    function clampValue(value, minimum, maximum) {
        return Math.max(minimum, Math.min(maximum, value));
    }

    function maximumContentX() {
        return Math.max(0, Math.max(imageFlickable.width, imageView.x + imageView.width) - imageFlickable.width);
    }

    function maximumContentY() {
        return Math.max(0, Math.max(imageFlickable.height, imageView.y + imageView.height) - imageFlickable.height);
    }

    function panBy(deltaX, deltaY) {
        if (!imagePannable) {
            return false;
        }

        const nextContentX = clampValue(imageFlickable.contentX + deltaX, 0, maximumContentX());
        const nextContentY = clampValue(imageFlickable.contentY + deltaY, 0, maximumContentY());
        const moved = nextContentX !== imageFlickable.contentX || nextContentY !== imageFlickable.contentY;
        imageFlickable.contentX = nextContentX;
        imageFlickable.contentY = nextContentY;
        return moved;
    }

    function viewportPointInsideImage(viewportX, viewportY) {
        if (!imageReady || imageView.width <= 0 || imageView.height <= 0) {
            return false;
        }

        const contentPointX = imageFlickable.contentX + viewportX;
        const contentPointY = imageFlickable.contentY + viewportY;
        return contentPointX >= imageView.x && contentPointX <= imageView.x + imageView.width && contentPointY >= imageView.y && contentPointY <= imageView.y + imageView.height;
    }

    function zoomBy(deltaPercent, viewportX, viewportY) {
        if (!imageReady) {
            return false;
        }

        const nextZoomPercent = clampValue(imageView.zoomPercent + deltaPercent, minimumManualZoomPercent, maximumManualZoomPercent);
        if (Math.abs(nextZoomPercent - imageView.zoomPercent) < 0.001) {
            return false;
        }

        const anchorViewportX = Number.isFinite(viewportX) ? clampValue(viewportX, 0, imageFlickable.width) : imageFlickable.width / 2;
        const anchorViewportY = Number.isFinite(viewportY) ? clampValue(viewportY, 0, imageFlickable.height) : imageFlickable.height / 2;
        const anchorRatioX = imageView.width > 0 ? clampValue((imageFlickable.contentX + anchorViewportX - imageView.x) / imageView.width, 0, 1) : 0.5;
        const anchorRatioY = imageView.height > 0 ? clampValue((imageFlickable.contentY + anchorViewportY - imageView.y) / imageView.height, 0, 1) : 0.5;

        imageView.zoomPercent = nextZoomPercent;
        imageFlickable.contentX = clampValue(imageView.x + anchorRatioX * imageView.width - anchorViewportX, 0, maximumContentX());
        imageFlickable.contentY = clampValue(imageView.y + anchorRatioY * imageView.height - anchorViewportY, 0, maximumContentY());
        return true;
    }

    Flickable {
        id: imageFlickable

        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        contentHeight: Math.max(height, imageView.y + imageView.height)
        contentWidth: Math.max(width, imageView.x + imageView.width)
        interactive: imageView.status === KiriImageView.Ready && (contentWidth > width || contentHeight > height)

        Controls.ScrollBar.horizontal: Controls.ScrollBar {
            policy: imageFlickable.contentWidth > imageFlickable.width ? Controls.ScrollBar.AsNeeded : Controls.ScrollBar.AlwaysOff
        }

        Controls.ScrollBar.vertical: Controls.ScrollBar {
            policy: imageFlickable.contentHeight > imageFlickable.height ? Controls.ScrollBar.AsNeeded : Controls.ScrollBar.AlwaysOff
        }

        KiriImageView {
            id: imageView

            height: displaySize.height
            viewportSize: Qt.size(imageFlickable.width, imageFlickable.height)
            width: displaySize.width
            x: Math.max(0, (imageFlickable.width - width) / 2)
            y: Math.max(0, (imageFlickable.height - height) / 2)

            Component.onCompleted: {
                if (root.initialSourceUrl.toString().length > 0) {
                    sourceUrl = root.initialSourceUrl;
                }
            }
        }
    }

    MouseArea {
        id: zoomDragArea

        anchors.fill: imageFlickable
        acceptedButtons: Qt.LeftButton
        enabled: root.imageReady

        property real lastY: 0
        property bool zoomDragActive: false

        onCanceled: zoomDragActive = false
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
