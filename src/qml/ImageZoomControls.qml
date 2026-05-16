// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

RowLayout {
    id: root

    required property KiriImageDocument imageDocument
    required property bool imageReady
    required property int minimumManualZoomPercent
    required property int maximumManualZoomPercent
    required property real zoomStepFactor
    property bool compact: false
    property real pendingZoomStepCount: 0
    readonly property int controlSpacing: compact ? Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2)) : Kirigami.Units.smallSpacing
    readonly property bool textInputActive: textInputFocused()

    signal editingCompleted(bool returnViewerFocus)

    function cancelEditing(returnViewerFocus) {
        if (textInputFocused()) {
            zoomSpinBox.cancelEditing(returnViewerFocus);
        }
    }

    function commitEditing(returnViewerFocus) {
        if (textInputFocused()) {
            zoomSpinBox.commitEditing(returnViewerFocus);
        }
    }

    spacing: controlSpacing

    function textInputFocused() {
        return zoomSpinBox.activeFocus || zoomTextInput.activeFocus;
    }

    function steppedZoomValue(stepCount) {
        return Math.round(root.imageDocument.steppedManualZoomPercent(stepCount));
    }

    function multiplicativeStepSize(stepCount) {
        return Math.max(1, Math.abs(root.steppedZoomValue(stepCount) - zoomSpinBox.value));
    }

    Controls.SpinBox {
        id: zoomSpinBox

        property bool completingEdit: false

        editable: true
        enabled: root.imageReady
        from: Math.min(root.minimumManualZoomPercent, Math.floor(root.imageDocument.zoomPercent))
        implicitWidth: Kirigami.Units.gridUnit * 5
        live: false
        stepSize: {
            if (zoomSpinBox.up.pressed) {
                return root.multiplicativeStepSize(1);
            }
            if (zoomSpinBox.down.pressed) {
                return root.multiplicativeStepSize(-1);
            }
            return Math.max(1, Math.round(root.imageDocument.zoomPercent * (root.zoomStepFactor - 1)));
        }
        to: Math.max(root.maximumManualZoomPercent, Math.ceil(root.imageDocument.zoomPercent))
        value: Math.round(root.imageDocument.zoomPercent)
        wheelEnabled: false

        textFromValue: function (value, locale) {
            return Number(value).toLocaleString(locale, "f", 0) + "%";
        }

        valueFromText: function (text, locale) {
            const withoutPercent = text.toString().replace("%", "").trim();
            const parsedValue = Number.fromLocaleString(locale, withoutPercent);
            return Number.isFinite(parsedValue) ? Math.round(parsedValue) : zoomSpinBox.value;
        }

        validator: IntValidator {
            bottom: zoomSpinBox.from
            top: zoomSpinBox.to
        }

        contentItem: TextInput {
            id: zoomTextInput

            color: zoomSpinBox.palette.text
            font: zoomSpinBox.font
            horizontalAlignment: Text.AlignHCenter
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            readOnly: !zoomSpinBox.editable
            selectByMouse: true
            selectedTextColor: zoomSpinBox.palette.highlightedText
            selectionColor: zoomSpinBox.palette.highlight
            verticalAlignment: Text.AlignVCenter

            Binding {
                property: "text"
                target: zoomTextInput
                value: zoomSpinBox.displayText
                when: !zoomTextInput.activeFocus
            }
        }

        function cancelEditing(returnViewerFocus) {
            if (completingEdit) {
                return;
            }

            completingEdit = true;
            restoreZoomText();
            clearEditingFocus();
            completingEdit = false;
            if (returnViewerFocus === true) {
                root.editingCompleted(true);
            }
        }

        function clearEditingFocus() {
            zoomTextInput.focus = false;
            zoomSpinBox.focus = false;
        }

        function commitEditing(returnViewerFocus) {
            if (completingEdit) {
                return;
            }

            completingEdit = true;
            commitZoomText();
            clearEditingFocus();
            completingEdit = false;
            if (returnViewerFocus === true) {
                root.editingCompleted(true);
            }
        }

        function commitZoomText() {
            if (!enabled) {
                restoreZoomText();
                return;
            }

            const zoomPercent = parsedZoomText();
            if (Number.isFinite(zoomPercent)) {
                root.pendingZoomStepCount = 0;
                root.imageDocument.zoomPercent = root.imageDocument.clampedManualZoomPercent(Math.round(zoomPercent));
            }
            restoreZoomText();
        }

        function parsedZoomText() {
            const withoutPercent = zoomText().replace("%", "").trim();
            if (withoutPercent.length === 0) {
                return NaN;
            }

            const parsedValue = Number.fromLocaleString(Qt.locale(), withoutPercent);
            return Number.isFinite(parsedValue) ? parsedValue : NaN;
        }

        function restoreZoomText() {
            zoomTextInput.text = zoomSpinBox.displayText;
        }

        function zoomText() {
            return zoomTextInput.text.toString();
        }

        onValueModified: {
            const stepCount = zoomSpinBox.up.pressed ? 1 : (zoomSpinBox.down.pressed ? -1 : root.pendingZoomStepCount);
            root.pendingZoomStepCount = 0;
            root.imageDocument.zoomPercent = stepCount === 0 ? root.imageDocument.clampedManualZoomPercent(value) : root.imageDocument.steppedManualZoomPercent(stepCount);
        }

        Connections {
            target: zoomSpinBox.up

            function onPressedChanged() {
                if (zoomSpinBox.up.pressed) {
                    root.pendingZoomStepCount = 1;
                }
            }
        }

        Connections {
            target: zoomSpinBox.down

            function onPressedChanged() {
                if (zoomSpinBox.down.pressed) {
                    root.pendingZoomStepCount = -1;
                }
            }
        }
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.textInputActive
        sequence: "Return"

        onActivated: root.commitEditing(true)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.textInputActive
        sequence: "Enter"

        onActivated: root.commitEditing(true)
    }
}
