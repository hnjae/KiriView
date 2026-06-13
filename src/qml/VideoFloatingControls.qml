// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.hnjae.kiriview
import org.kde.ki18n
import org.kde.kirigami as Kirigami

Controls.Control {
    id: root

    objectName: "videoPlaybackControls"

    required property KiriVideoDocument videoDocument
    property bool fixedMode: false
    property real durationMs: 0
    property real positionMs: 0
    property bool explicitlyRevealed: true
    readonly property bool validDuration: durationMs > 0
    readonly property bool timelineInteractive: videoDocument.seekable && validDuration
    readonly property string positionText: formatTimestamp(timelineSlider.pressed ? timelineSlider.sliderPosition : positionMs)
    readonly property string durationText: validDuration ? formatTimestamp(durationMs) : "--:--"
    readonly property bool interactionActive: controlsHoverHandler.hovered || playPauseButton.pressed || playPauseButton.activeFocus || timelineSlider.pressed || timelineSlider.activeFocus || muteButton.pressed || muteButton.activeFocus
    readonly property bool autoHideEligible: !fixedMode && videoDocument.playing
    readonly property bool controlsShown: fixedMode || !videoDocument.playing || interactionActive || explicitlyRevealed
    readonly property real horizontalViewportMargin: Kirigami.Units.largeSpacing * 2
    readonly property real availableResponsiveWidth: parent ? Math.max(0, parent.width - horizontalViewportMargin) : implicitWidth
    readonly property real preferredResponsiveWidth: parent ? parent.width * 0.75 : implicitWidth
    readonly property real floatingWidth: parent ? Math.min(availableResponsiveWidth, Math.max(implicitWidth, preferredResponsiveWidth)) : implicitWidth

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

    function scheduleAutoHide() {
        if (root.autoHideEligible && !root.interactionActive) {
            hideTimer.restart();
            return;
        }
        hideTimer.stop();
    }

    function revealControls() {
        if (!root.visible) {
            return;
        }

        root.explicitlyRevealed = true;
        scheduleAutoHide();
    }

    leftPadding: Kirigami.Units.smallSpacing
    rightPadding: Kirigami.Units.smallSpacing
    topPadding: Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2))
    bottomPadding: Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2))
    enabled: controlsShown
    opacity: controlsShown ? 1 : 0
    width: fixedMode && parent ? parent.width : floatingWidth

    onPositionMsChanged: syncTimelineToDocument()
    onTimelineInteractiveChanged: syncTimelineToDocument()
    onDurationMsChanged: syncTimelineToDocument()
    onVideoDocumentChanged: syncDocumentTiming()
    onAutoHideEligibleChanged: {
        explicitlyRevealed = true;
        scheduleAutoHide();
    }
    onFixedModeChanged: {
        explicitlyRevealed = true;
        scheduleAutoHide();
    }
    onInteractionActiveChanged: {
        if (interactionActive) {
            revealControls();
            return;
        }
        scheduleAutoHide();
    }
    onVisibleChanged: {
        if (visible) {
            revealControls();
        }
    }

    Behavior on opacity {
        enabled: !root.fixedMode

        NumberAnimation {
            duration: Kirigami.Units.shortDuration
            easing.type: Easing.InOutQuad
        }
    }

    Component.onCompleted: {
        syncDocumentTiming();
        scheduleAutoHide();
    }

    Connections {
        target: root.videoDocument

        function onDurationChanged() {
            root.syncDocumentTiming();
        }

        function onPositionChanged() {
            root.syncDocumentPosition();
        }

        function onPlayingChanged() {
            root.revealControls();
        }
    }

    HoverHandler {
        id: controlsHoverHandler
    }

    Timer {
        id: hideTimer

        interval: Kirigami.Units.humanMoment
        repeat: false

        onTriggered: {
            if (root.autoHideEligible && !root.interactionActive) {
                root.explicitlyRevealed = false;
            }
        }
    }

    background: Kirigami.ShadowedRectangle {
        color: Kirigami.Theme.backgroundColor
        opacity: root.fixedMode ? 0.96 : 0.84
        radius: root.fixedMode ? 0 : Kirigami.Units.cornerRadius

        border.color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, Kirigami.Theme.frameContrast)
        border.width: 1

        shadow.color: Qt.rgba(0, 0, 0, root.fixedMode ? 0 : 0.18)
        shadow.size: root.fixedMode ? 0 : Kirigami.Units.smallSpacing
        shadow.xOffset: 0
        shadow.yOffset: root.fixedMode ? 0 : Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2))
    }

    contentItem: RowLayout {
        spacing: Kirigami.Units.smallSpacing

        Controls.ToolButton {
            id: playPauseButton

            objectName: "videoPlaybackPlayPauseButton"

            Accessible.name: text
            Accessible.role: Accessible.Button
            display: Controls.AbstractButton.IconOnly
            icon.name: root.videoDocument.playing ? "media-playback-pause-symbolic" : "media-playback-start-symbolic"
            text: root.videoDocument.playing ? KI18n.i18nc("@action:button", "Pause") : KI18n.i18nc("@action:button", "Play")

            Controls.ToolTip.text: text
            Controls.ToolTip.visible: hovered && Controls.ToolTip.text.length > 0 && !Kirigami.Settings.hasTransientTouchInput

            onPressedChanged: {
                if (pressed) {
                    root.revealControls();
                }
            }
            onClicked: root.videoDocument.togglePlayback()
        }

        Controls.Label {
            id: currentTimeLabel

            objectName: "videoPlaybackCurrentTimeLabel"

            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: Kirigami.Units.gridUnit * 4
            color: Kirigami.Theme.textColor
            elide: Text.ElideRight
            font: Kirigami.Theme.fixedWidthFont
            fontSizeMode: Text.HorizontalFit
            horizontalAlignment: Text.AlignRight
            maximumLineCount: 1
            minimumPixelSize: Math.max(8, Kirigami.Theme.smallFont.pixelSize - 3)
            text: root.positionText
        }

        Controls.Slider {
            id: timelineSlider

            objectName: "videoPlaybackSlider"

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
                    root.revealControls();
                    sliderPosition = root.positionMs;
                    return;
                }
                root.commitTimelineSeek();
                root.scheduleAutoHide();
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
            id: durationTimeLabel

            objectName: "videoPlaybackDurationLabel"

            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: Kirigami.Units.gridUnit * 4
            color: Kirigami.Theme.textColor
            elide: Text.ElideRight
            font: Kirigami.Theme.fixedWidthFont
            fontSizeMode: Text.HorizontalFit
            horizontalAlignment: Text.AlignLeft
            maximumLineCount: 1
            minimumPixelSize: Math.max(8, Kirigami.Theme.smallFont.pixelSize - 3)
            text: root.durationText
        }

        Controls.ToolButton {
            id: muteButton

            objectName: "videoPlaybackMuteButton"

            Accessible.name: text
            Accessible.role: Accessible.Button
            display: Controls.AbstractButton.IconOnly
            icon.name: root.videoDocument.muted ? "audio-volume-muted-symbolic" : "audio-volume-high-symbolic"
            text: root.videoDocument.muted ? KI18n.i18nc("@action:button", "Unmute") : KI18n.i18nc("@action:button", "Mute")

            Controls.ToolTip.text: text
            Controls.ToolTip.visible: hovered && Controls.ToolTip.text.length > 0 && !Kirigami.Settings.hasTransientTouchInput

            onPressedChanged: {
                if (pressed) {
                    root.revealControls();
                }
            }
            onClicked: root.videoDocument.toggleMuted()
        }
    }
}
