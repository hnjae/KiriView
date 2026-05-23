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
    property bool readOnlyDisplayMode: false
    property bool readOnlyPercentKnown: false
    property int readOnlyPercent: 0
    property bool zoomPercentAvailable: readOnlyDisplayMode || imageReady
    property bool zoomPercentKnown: readOnlyDisplayMode ? readOnlyPercentKnown : imageReady
    property real zoomPercent: readOnlyDisplayMode ? readOnlyPercent : imageDocument.zoomPercent
    property bool zoomEditable: !readOnlyDisplayMode && imageReady
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

        objectName: "zoomSpinBox"

        property bool completingEdit: false
        readonly property int zoomDisplayWidth: 7
        readonly property real zoomDisplayEpsilon: 0.001
        readonly property int zoomKiloThresholdPercent: 10000
        readonly property int zoomOverflowThresholdPercent: 1000000
        readonly property bool zoomValueAvailable: root.zoomPercentAvailable
        readonly property bool zoomValueKnown: root.zoomPercentKnown
        readonly property real rawZoomPercent: root.zoomPercent
        readonly property real numericZoomPercent: Number.isFinite(Number(rawZoomPercent)) ? Number(rawZoomPercent) : 0
        readonly property string formattedDisplayText: formattedZoomText(rawZoomPercent, zoomValueAvailable, zoomValueKnown)
        readonly property string editableDisplayText: plainZoomText(value)

        editable: root.zoomEditable
        enabled: root.zoomEditable
        from: root.zoomEditable ? Math.min(root.minimumManualZoomPercent, Math.floor(numericZoomPercent)) : 0
        implicitWidth: Kirigami.Units.gridUnit * 5
        live: false
        stepSize: {
            if (zoomSpinBox.up.pressed) {
                return root.multiplicativeStepSize(1);
            }
            if (zoomSpinBox.down.pressed) {
                return root.multiplicativeStepSize(-1);
            }
            return Math.max(1, Math.round(numericZoomPercent * (root.zoomStepFactor - 1)));
        }
        to: root.zoomEditable ? Math.max(root.maximumManualZoomPercent, Math.ceil(numericZoomPercent)) : Math.max(0, Math.round(numericZoomPercent))
        value: zoomValueAvailable && zoomValueKnown ? Math.max(0, Math.round(numericZoomPercent)) : 0
        wheelEnabled: false

        textFromValue: function (value, locale) {
            return plainZoomText(value);
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

            objectName: "zoomTextInput"

            color: zoomSpinBox.palette.text
            font: Kirigami.Theme.fixedWidthFont
            horizontalAlignment: Text.AlignRight
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            readOnly: !root.zoomEditable || !zoomSpinBox.editable
            selectByMouse: true
            selectedTextColor: zoomSpinBox.palette.highlightedText
            selectionColor: zoomSpinBox.palette.highlight
            verticalAlignment: Text.AlignVCenter

            Binding {
                property: "text"
                target: zoomTextInput
                value: zoomSpinBox.formattedDisplayText
                when: !root.zoomEditable || !zoomTextInput.activeFocus
            }

            onActiveFocusChanged: {
                if (activeFocus && root.zoomEditable && zoomSpinBox.enabled) {
                    text = zoomSpinBox.editableDisplayText;
                }
            }
        }

        function formattedZoomText(rawPercent, valueAvailable, valueKnown) {
            if (!valueAvailable) {
                return paddedZoomText("- %");
            }

            const percent = Number(rawPercent);
            if (!valueKnown || !Number.isFinite(percent)) {
                return paddedZoomText("? %");
            }

            if (percent < zoomKiloThresholdPercent) {
                const roundedPercent = Math.min(zoomKiloThresholdPercent - 1, Math.max(0, Math.round(percent)));
                return paddedZoomText(roundedPercent.toString() + " %");
            }

            if (percent >= zoomOverflowThresholdPercent) {
                return paddedZoomText("999k+ %");
            }

            const nearestKilo = Math.round(percent / 1000);
            const nearestKiloPercent = nearestKilo * 1000;
            if (Math.abs(percent - nearestKiloPercent) < zoomDisplayEpsilon) {
                if (nearestKilo >= 1000) {
                    return paddedZoomText("999k+ %");
                }
                return paddedZoomText(nearestKilo.toString() + "k %");
            }

            const kilo = Math.min(999, Math.floor(percent / 1000));
            return paddedZoomText(kilo.toString() + "k+ %");
        }

        function paddedZoomText(text) {
            let paddedText = text.toString();
            while (paddedText.length < zoomDisplayWidth) {
                paddedText = " " + paddedText;
            }
            return paddedText;
        }

        function plainZoomText(value) {
            return Number(value).toString() + " %";
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
            if (!root.zoomEditable || !enabled) {
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
            zoomTextInput.text = zoomSpinBox.formattedDisplayText;
        }

        function zoomText() {
            return zoomTextInput.text.toString();
        }

        onValueModified: {
            if (!root.zoomEditable) {
                return;
            }

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
