// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import io.github.hnjae.kiriview
import org.kde.ki18n
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as KirigamiAddons

KirigamiAddons.FloatingToolBar {
    id: root

    required property KiriVideoDocument videoDocument
    property real durationMs: 0
    property real positionMs: 0
    readonly property bool validDuration: durationMs > 0
    readonly property bool timelineInteractive: videoDocument.seekable && validDuration
    readonly property string positionText: formatTimestamp(timelineSlider.pressed ? timelineSlider.sliderPosition : positionMs)
    readonly property string durationText: validDuration ? formatTimestamp(durationMs) : "--:--"
    readonly property bool interactionActive: controlsHoverHandler.hovered || activeFocus
    readonly property real horizontalViewportMargin: Kirigami.Units.largeSpacing * 2
    readonly property real availableResponsiveWidth: parent ? Math.max(0, parent.width - horizontalViewportMargin) : implicitWidth
    readonly property real minimumResponsiveWidth: Math.max(implicitWidth, Kirigami.Units.gridUnit * 24)
    readonly property real preferredResponsiveWidth: parent ? parent.width * 0.65 : implicitWidth
    readonly property real maximumResponsiveWidth: Kirigami.Units.gridUnit * 44

    function documentMilliseconds(propertyName) {
        const value = Number(root.videoDocument[propertyName]);
        return Number.isFinite(value) && value > 0 ? value : 0;
    }

    function syncDocumentTiming() {
        root.durationMs = documentMilliseconds("duration");
        root.positionMs = clampedPosition(documentMilliseconds("position"));
        syncTimelineToDocument();
    }

    function syncDocumentPosition() {
        root.positionMs = clampedPosition(documentMilliseconds("position"));
        syncTimelineToDocument();
    }

    function clampedPosition(position) {
        if (!Number.isFinite(position) || position <= 0) {
            return 0;
        }
        if (!root.validDuration) {
            return position;
        }
        return Math.min(position, root.durationMs);
    }

    function formatTimestamp(milliseconds) {
        if (!Number.isFinite(milliseconds) || milliseconds < 0) {
            return "--:--";
        }

        const totalSeconds = Math.floor(milliseconds / 1000);
        const hours = Math.floor(totalSeconds / 3600);
        const minutes = Math.floor((totalSeconds % 3600) / 60);
        const seconds = totalSeconds % 60;
        const paddedSeconds = seconds.toString().padStart(2, "0");
        if (hours > 0) {
            return hours.toString() + ":" + minutes.toString().padStart(2, "0") + ":" + paddedSeconds;
        }
        return minutes.toString() + ":" + paddedSeconds;
    }

    function syncTimelineToDocument() {
        if (timelineSlider.pressed) {
            return;
        }
        timelineSlider.sliderPosition = root.timelineInteractive ? root.positionMs : 0;
    }

    function commitTimelineSeek() {
        if (!root.timelineInteractive) {
            syncTimelineToDocument();
            return;
        }

        const nextPosition = Math.round(root.clampedPosition(timelineSlider.sliderPosition));
        timelineSlider.sliderPosition = nextPosition;
        root.videoDocument.setPosition(nextPosition);
    }

    leftPadding: Kirigami.Units.smallSpacing
    rightPadding: Kirigami.Units.smallSpacing
    topPadding: Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2))
    bottomPadding: Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2))
    width: parent ? Math.min(availableResponsiveWidth, Math.max(minimumResponsiveWidth, Math.min(preferredResponsiveWidth, maximumResponsiveWidth))) : implicitWidth

    onPositionMsChanged: syncTimelineToDocument()
    onTimelineInteractiveChanged: syncTimelineToDocument()
    onDurationMsChanged: syncTimelineToDocument()
    onVideoDocumentChanged: syncDocumentTiming()

    Component.onCompleted: syncDocumentTiming()

    Connections {
        target: root.videoDocument

        function onDurationChanged() {
            root.syncDocumentTiming();
        }

        function onPositionChanged() {
            root.syncDocumentPosition();
        }
    }

    HoverHandler {
        id: controlsHoverHandler
    }

    contentItem: RowLayout {
        spacing: Kirigami.Units.smallSpacing

        Controls.ToolButton {
            id: playPauseButton

            Accessible.name: text
            Accessible.role: Accessible.Button
            display: Controls.AbstractButton.IconOnly
            icon.name: root.videoDocument.playing ? "media-playback-pause-symbolic" : "media-playback-start-symbolic"
            text: root.videoDocument.playing ? KI18n.i18nc("@action:button", "Pause") : KI18n.i18nc("@action:button", "Play")

            Controls.ToolTip.text: text
            Controls.ToolTip.visible: hovered && Controls.ToolTip.text.length > 0 && !Kirigami.Settings.hasTransientTouchInput

            onClicked: root.videoDocument.togglePlayback()
        }

        Controls.Slider {
            id: timelineSlider

            property real sliderPosition: 0

            Accessible.name: KI18n.i18nc("@label:slider", "Position")
            Layout.fillWidth: true
            Layout.minimumWidth: Kirigami.Units.gridUnit * 4
            enabled: root.timelineInteractive
            from: 0
            live: false
            stepSize: 1000
            to: root.timelineInteractive ? root.durationMs : 1
            value: root.timelineInteractive ? sliderPosition : 0

            onMoved: sliderPosition = root.clampedPosition(value)
            onPressedChanged: {
                if (pressed) {
                    sliderPosition = root.positionMs;
                    return;
                }
                root.commitTimelineSeek();
            }

            Keys.priority: Keys.AfterItem
            Keys.onPressed: event => {
                if (event.modifiers !== Qt.NoModifier) {
                    return;
                }

                switch (event.key) {
                case Qt.Key_Left:
                case Qt.Key_Right:
                case Qt.Key_Up:
                case Qt.Key_Down:
                    event.accepted = true;
                    break;
                default:
                    break;
                }
            }
        }

        Controls.Label {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: Kirigami.Units.gridUnit * 5
            color: Kirigami.Theme.textColor
            elide: Text.ElideRight
            font: Kirigami.Theme.smallFont
            fontSizeMode: Text.HorizontalFit
            horizontalAlignment: Text.AlignRight
            maximumLineCount: 1
            minimumPixelSize: Math.max(8, Kirigami.Theme.smallFont.pixelSize - 3)
            text: root.positionText + " / " + root.durationText
        }
    }
}
