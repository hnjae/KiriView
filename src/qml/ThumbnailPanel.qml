// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import io.github.hnjae.kiriview
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
            readonly property int rapidCurrentIndexIntervalMs: 180
            property bool automaticScrollAnimationEnabled: false
            property bool automaticScrollAnimationRunning: false
            property double lastCurrentIndexChangeTimestamp: 0

            function containCurrentItem(forceImmediate) {
                if (currentIndex < 0 || currentIndex >= count) {
                    automaticScrollAnimationEnabled = false;
                    return;
                }

                automaticScrollAnimationEnabled = shouldAnimateContainment(forceImmediate !== true);
                positionViewAtIndex(currentIndex, ListView.Contain);
            }

            function containCurrentItemForNavigationIntent(forceImmediate) {
                switch (root.documentSession.activeNavigationRevealIntent) {
                case KiriDocumentSession.AdjacentNavigation:
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

            function currentIndexChangedRapidly() {
                const now = Date.now();
                const rapid = lastCurrentIndexChangeTimestamp > 0 && now - lastCurrentIndexChangeTimestamp < rapidCurrentIndexIntervalMs;
                lastCurrentIndexChangeTimestamp = now;
                return rapid;
            }

            function shouldAnimateContainment(allowAnimation) {
                if (!allowAnimation || !root.visible || width <= 0 || automaticScrollAnimationRunning) {
                    return false;
                }

                const delta = containmentDeltaForCurrentItem();
                if (delta === 0) {
                    return false;
                }

                const nearThreshold = Math.max(itemPitch, Math.min(width * 0.5, itemPitch * 3));
                return Math.abs(delta) <= nearThreshold;
            }

            onCountChanged: containCurrentItem(true)
            onCurrentIndexChanged: containCurrentItemForNavigationIntent(currentIndexChangedRapidly())
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
                required property int number

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

                    Kirigami.Icon {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredHeight: Kirigami.Units.iconSizes.large
                        Layout.preferredWidth: Kirigami.Units.iconSizes.large
                        color: root.viewerForegroundColor
                        source: thumbnailDelegate.iconName
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
