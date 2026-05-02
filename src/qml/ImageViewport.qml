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
    readonly property int minimumManualZoomPercent: imageView.minimumManualZoomPercent
    readonly property int maximumManualZoomPercent: imageView.maximumManualZoomPercent
    readonly property int zoomStepPercent: imageView.zoomStepPercent
    readonly property real dragZoomPercentPerPixel: zoomStepPercent / Math.max(1, Kirigami.Units.gridUnit * 2)
    readonly property bool imagePannable: imageFlickable.contentWidth > imageFlickable.width || imageFlickable.contentHeight > imageFlickable.height
    readonly property real viewportWidth: imageFlickable.width
    readonly property real viewportHeight: imageFlickable.height

    function setContentPosition(contentPosition) {
        imageFlickable.contentX = contentPosition.x;
        imageFlickable.contentY = contentPosition.y;
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

        const nextZoomPercent = imageView.clampedManualZoomPercent(imageView.zoomPercent + deltaPercent);
        if (Math.abs(nextZoomPercent - imageView.zoomPercent) < 0.001) {
            return false;
        }

        const nextContentPosition = imageView.zoomContentPosition(Qt.point(imageFlickable.contentX, imageFlickable.contentY), Qt.point(viewportX, viewportY), nextZoomPercent);
        imageView.zoomPercent = nextZoomPercent;
        setContentPosition(nextContentPosition);
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
