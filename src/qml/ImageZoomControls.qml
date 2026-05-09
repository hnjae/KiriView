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

    spacing: controlSpacing

    function textInputFocused() {
        return zoomSpinBox.activeFocus || (zoomSpinBox.contentItem !== null && zoomSpinBox.contentItem.activeFocus);
    }

    function steppedZoomValue(stepCount) {
        return Math.round(root.imageDocument.steppedManualZoomPercent(stepCount));
    }

    function multiplicativeStepSize(stepCount) {
        return Math.max(1, Math.abs(root.steppedZoomValue(stepCount) - zoomSpinBox.value));
    }

    Controls.SpinBox {
        id: zoomSpinBox

        editable: true
        enabled: root.imageReady
        from: Math.min(root.minimumManualZoomPercent, Math.floor(root.imageDocument.zoomPercent))
        implicitWidth: Kirigami.Units.gridUnit * 5
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
}
