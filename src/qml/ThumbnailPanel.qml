// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQuick.Window
import org.hnjae.kiriview
import org.kde.kirigami as Kirigami

Controls.Pane {
    id: root

    required property KiriDocumentSession documentSession
    required property color viewerForegroundColor
    required property color viewerSurfaceColor

    leftPadding: Kirigami.Units.smallSpacing
    rightPadding: Kirigami.Units.smallSpacing
    topPadding: Kirigami.Units.smallSpacing
    bottomPadding: Kirigami.Units.smallSpacing

    onVisibleChanged: {
        if (visible) {
            thumbnailStrip.containCurrentItem(true);
        } else {
            thumbnailStrip.automaticScrollAnimationEnabled = false;
        }
    }

    background: Rectangle {
        color: root.viewerSurfaceColor

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            color: root.viewerForegroundColor
            height: 1
            opacity: 0.18
        }
    }

    contentItem: ColumnLayout {
        spacing: Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2))

        ListView {
            id: thumbnailStrip

            objectName: "thumbnailStrip"

            Layout.fillHeight: true
            Layout.fillWidth: true
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            currentIndex: root.documentSession.activeNavigationCurrentNumber - 1
            model: root.documentSession.activeNavigationThumbnailModel
            orientation: ListView.Horizontal
            spacing: Kirigami.Units.smallSpacing

            readonly property real delegateWidth: Kirigami.Units.gridUnit * 6
            readonly property real itemPitch: delegateWidth + spacing
            readonly property real preferredZoneInset: Math.min(itemPitch, Math.max(0, (width - delegateWidth) / 2), Math.max(0, width * 0.25))
            readonly property real preferredZoneStart: preferredZoneInset
            readonly property real preferredZoneEnd: Math.max(preferredZoneStart + Math.min(delegateWidth, width), width - preferredZoneInset)
            readonly property int rapidCurrentIndexIntervalMs: 180
            readonly property int userScrollSuppressionIntervalMs: 700
            property bool automaticScrollAnimationEnabled: false
            property bool automaticScrollAnimationRunning: false
            property double lastCurrentIndexChangeTimestamp: 0
            property real preferredZoneSnapPosition: 0
            property double userScrollSuppressionUntilTimestamp: 0

            preferredHighlightBegin: preferredZoneSnapPosition
            preferredHighlightEnd: preferredZoneSnapPosition + delegateWidth

            function containCurrentItem(forceImmediate) {
                if (currentIndex < 0 || currentIndex >= count) {
                    automaticScrollAnimationEnabled = false;
                    return;
                }

                automaticScrollAnimationEnabled = shouldAnimateContainment(forceImmediate !== true);
                positionViewAtIndex(currentIndex, ListView.Contain);
            }

            function followCurrentItemInPreferredZone(forceImmediate) {
                if (currentIndex < 0 || currentIndex >= count) {
                    automaticScrollAnimationEnabled = false;
                    return;
                }

                if (userScrollSuppressionActive()) {
                    const containmentDelta = containmentDeltaForCurrentItem();
                    if (containmentDelta === 0) {
                        automaticScrollAnimationEnabled = false;
                        return;
                    }

                    automaticScrollAnimationEnabled = shouldAnimateReveal(containmentDelta, forceImmediate !== true);
                    positionViewAtIndex(currentIndex, ListView.Contain);
                    return;
                }

                const delta = preferredZoneDeltaForCurrentItem();
                if (delta === 0) {
                    automaticScrollAnimationEnabled = false;
                    return;
                }

                preferredZoneSnapPosition = preferredZoneSnapPositionForCurrentReveal(delta);

                automaticScrollAnimationEnabled = shouldAnimateReveal(delta, forceImmediate !== true);
                positionViewAtIndex(currentIndex, ListView.SnapPosition);
            }

            function preferredZoneSnapPositionForCurrentReveal(delta) {
                const trailingSnapPosition = Math.max(preferredZoneStart, preferredZoneEnd - delegateWidth);
                switch (root.documentSession.activeNavigationRevealDirection) {
                case KiriDocumentSession.Next:
                    return preferredZoneStart;
                case KiriDocumentSession.Previous:
                    return trailingSnapPosition;
                default:
                    return delta < 0 ? preferredZoneStart : trailingSnapPosition;
                }
            }

            function containCurrentItemForNavigationIntent(forceImmediate) {
                switch (root.documentSession.activeNavigationRevealIntent) {
                case KiriDocumentSession.AdjacentNavigation:
                    followCurrentItemInPreferredZone(forceImmediate);
                    return;
                case KiriDocumentSession.LargeJump:
                case KiriDocumentSession.LoadOrOpen:
                    containCurrentItem(forceImmediate);
                    return;
                default:
                    automaticScrollAnimationEnabled = false;
                    return;
                }
            }

            function containmentDeltaForCurrentItem() {
                if (width <= 0 || contentWidth <= 0 || delegateWidth <= 0) {
                    return 0;
                }

                const itemStart = currentIndex * itemPitch;
                const itemEnd = itemStart + delegateWidth;
                const viewportStart = contentX;
                const viewportEnd = viewportStart + width;
                let delta = 0;

                if (itemStart < viewportStart) {
                    delta = itemStart - viewportStart;
                } else if (itemEnd > viewportEnd) {
                    delta = itemEnd - viewportEnd;
                }

                if (delta === 0) {
                    return 0;
                }

                const maxViewportStart = Math.max(0, contentWidth - width);
                const targetViewportStart = Math.max(0, Math.min(maxViewportStart, viewportStart + delta));
                return targetViewportStart - viewportStart;
            }

            function preferredZoneDeltaForCurrentItem() {
                if (width <= 0 || contentWidth <= 0 || delegateWidth <= 0) {
                    return 0;
                }

                const itemStart = currentIndex * itemPitch;
                const itemEnd = itemStart + delegateWidth;
                const viewportStart = contentX;
                const itemStartInViewport = itemStart - viewportStart;
                const itemEndInViewport = itemEnd - viewportStart;
                let delta = 0;

                if (itemStartInViewport < preferredZoneStart) {
                    delta = itemStartInViewport - preferredZoneStart;
                } else if (itemEndInViewport > preferredZoneEnd) {
                    delta = itemEndInViewport - preferredZoneEnd;
                }

                if (delta === 0) {
                    return 0;
                }

                const maxViewportStart = Math.max(0, contentWidth - width);
                const targetViewportStart = Math.max(0, Math.min(maxViewportStart, viewportStart + delta));
                return targetViewportStart - viewportStart;
            }

            function currentIndexChangedRapidly() {
                const now = Date.now();
                const rapid = lastCurrentIndexChangeTimestamp > 0 && now - lastCurrentIndexChangeTimestamp < rapidCurrentIndexIntervalMs;
                lastCurrentIndexChangeTimestamp = now;
                return rapid;
            }

            function shouldAnimateContainment(allowAnimation) {
                return shouldAnimateReveal(containmentDeltaForCurrentItem(), allowAnimation);
            }

            function shouldAnimateReveal(delta, allowAnimation) {
                if (!allowAnimation || !root.visible || width <= 0 || automaticScrollAnimationRunning) {
                    return false;
                }

                if (delta === 0) {
                    return false;
                }

                const nearThreshold = Math.max(itemPitch, Math.min(width * 0.5, itemPitch * 3));
                return Math.abs(delta) <= nearThreshold;
            }

            function noteUserThumbnailScroll() {
                automaticScrollAnimationEnabled = false;
                userScrollSuppressionUntilTimestamp = Date.now() + userScrollSuppressionIntervalMs;
            }

            function userScrollSuppressionActive() {
                return Date.now() < userScrollSuppressionUntilTimestamp;
            }

            onCountChanged: containCurrentItem(true)
            onCurrentIndexChanged: containCurrentItemForNavigationIntent(currentIndexChangedRapidly())
            onDraggingChanged: {
                if (dragging) {
                    noteUserThumbnailScroll();
                }
            }
            onWidthChanged: containCurrentItem(true)

            Behavior on contentX {
                enabled: thumbnailStrip.automaticScrollAnimationEnabled

                NumberAnimation {
                    duration: 140
                    easing.type: Easing.OutCubic

                    onRunningChanged: {
                        thumbnailStrip.automaticScrollAnimationRunning = running;
                        if (!running) {
                            thumbnailStrip.automaticScrollAnimationEnabled = false;
                        }
                    }
                }
            }

            Controls.ScrollBar.horizontal: Controls.ScrollBar {
                id: thumbnailScrollBar

                parent: horizontalScrollBarLane
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: implicitHeight

                active: hovered || pressed || thumbnailStrip.moving
                hoverEnabled: true
                policy: Controls.ScrollBar.AsNeeded
            }

            delegate: Controls.ItemDelegate {
                id: thumbnailDelegate

                required property bool current
                required property string iconName
                required property string label
                required property var navigationGeneration
                required property int number
                required property url thumbnailImageSource
                required property int thumbnailStatus
                required property url url

                objectName: "thumbnailStripItem"

                Accessible.name: label
                Accessible.role: Accessible.Button
                enabled: root.documentSession.activeNavigationEditable
                height: thumbnailStrip.height
                hoverEnabled: true
                leftPadding: Kirigami.Units.smallSpacing
                rightPadding: Kirigami.Units.smallSpacing
                topPadding: Kirigami.Units.smallSpacing
                bottomPadding: Kirigami.Units.smallSpacing
                width: thumbnailStrip.delegateWidth

                Controls.ToolTip.text: label
                Controls.ToolTip.visible: hovered && label.length > 0 && !Kirigami.Settings.hasTransientTouchInput

                // qmllint disable missing-property
                readonly property real thumbnailDevicePixelRatio: Window.window && Window.window.effectiveDevicePixelRatio > 0 ? Window.window.effectiveDevicePixelRatio : 1.0
                // qmllint enable missing-property
                readonly property bool thumbnailImageReady: thumbnailStatus === KiriDocumentSession.ReadyThumbnailResult && thumbnailImageSource.toString().length > 0

                function previewBoxIntersectsViewport() {
                    if (thumbnailPreviewBox.width <= 0 || thumbnailPreviewBox.height <= 0 || thumbnailStrip.width <= 0 || thumbnailStrip.height <= 0) {
                        return false;
                    }

                    const previewPosition = thumbnailPreviewBox.mapToItem(thumbnailStrip.contentItem, 0, 0);
                    const previewLeft = previewPosition.x;
                    const previewRight = previewLeft + thumbnailPreviewBox.width;
                    const previewTop = previewPosition.y;
                    const previewBottom = previewTop + thumbnailPreviewBox.height;
                    const viewportLeft = thumbnailStrip.contentX;
                    const viewportRight = viewportLeft + thumbnailStrip.width;
                    const viewportTop = thumbnailStrip.contentY;
                    const viewportBottom = viewportTop + thumbnailStrip.height;
                    return previewRight > viewportLeft && previewLeft < viewportRight && previewBottom > viewportTop && previewTop < viewportBottom;
                }

                function reportThumbnailDemand() {
                    const physicalMaxEdge = Math.ceil(Math.max(thumbnailPreviewBox.width, thumbnailPreviewBox.height) * thumbnailDevicePixelRatio);
                    const bucket = root.documentSession.activeNavigationThumbnailDemandBucket(physicalMaxEdge);
                    if (bucket === KiriDocumentSession.NoThumbnailDemandBucket) {
                        return;
                    }

                    const priority = previewBoxIntersectsViewport() ? KiriDocumentSession.VisibleThumbnailDemand : KiriDocumentSession.NearbyThumbnailDemand;
                    root.documentSession.reportActiveNavigationThumbnailDemand(number, url, physicalMaxEdge, priority, navigationGeneration);
                }

                Component.onCompleted: reportThumbnailDemand()
                onNavigationGenerationChanged: reportThumbnailDemand()
                onThumbnailDevicePixelRatioChanged: reportThumbnailDemand()
                onXChanged: reportThumbnailDemand()

                background: Item {
                    Rectangle {
                        anchors.fill: parent
                        color: root.viewerForegroundColor
                        opacity: thumbnailDelegate.hovered ? 0.08 : 0
                        radius: 3
                    }

                    Rectangle {
                        anchors.fill: parent
                        border.color: thumbnailDelegate.current ? Kirigami.Theme.highlightColor : "transparent"
                        border.width: thumbnailDelegate.current ? 2 : 0
                        color: "transparent"
                        radius: 3
                    }
                }

                contentItem: ColumnLayout {
                    spacing: Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2))

                    Item {
                        id: thumbnailPreviewBox

                        objectName: "thumbnailPreviewBox"

                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        Layout.minimumHeight: Kirigami.Units.iconSizes.large

                        onHeightChanged: thumbnailDelegate.reportThumbnailDemand()
                        onWidthChanged: thumbnailDelegate.reportThumbnailDemand()

                        Image {
                            anchors.fill: parent
                            asynchronous: true
                            fillMode: Image.PreserveAspectFit
                            mipmap: true
                            smooth: true
                            source: thumbnailDelegate.thumbnailImageSource
                            visible: thumbnailDelegate.thumbnailImageReady
                        }

                        Kirigami.Icon {
                            anchors.centerIn: parent
                            color: root.viewerForegroundColor
                            height: Math.min(parent.height, Kirigami.Units.iconSizes.large)
                            source: thumbnailDelegate.iconName
                            visible: !thumbnailDelegate.thumbnailImageReady
                            width: Math.min(parent.width, Kirigami.Units.iconSizes.large)
                        }
                    }

                    Controls.Label {
                        Layout.fillWidth: true
                        color: root.viewerForegroundColor
                        elide: Text.ElideRight
                        font: Kirigami.Theme.fixedWidthFont
                        horizontalAlignment: Text.AlignHCenter
                        maximumLineCount: 1
                        text: thumbnailDelegate.label
                        textFormat: Text.PlainText
                        wrapMode: Text.NoWrap
                    }
                }

                onClicked: root.documentSession.openActiveNavigationThumbnailAtNumber(number)

                Connections {
                    target: thumbnailStrip

                    function onContentXChanged() {
                        thumbnailDelegate.reportThumbnailDemand();
                    }

                    function onContentYChanged() {
                        thumbnailDelegate.reportThumbnailDemand();
                    }

                    function onHeightChanged() {
                        thumbnailDelegate.reportThumbnailDemand();
                    }

                    function onWidthChanged() {
                        thumbnailDelegate.reportThumbnailDemand();
                    }
                }
            }
        }

        Item {
            id: horizontalScrollBarLane

            readonly property real laneHeight: Math.max(thumbnailScrollBar.implicitHeight, Kirigami.Units.smallSpacing)

            Layout.fillWidth: true
            Layout.minimumHeight: laneHeight
            Layout.preferredHeight: laneHeight
        }
    }
}
