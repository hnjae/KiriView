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
    required property int zoomStepPercent
    property bool compact: false
    readonly property int controlSpacing: compact ? Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2)) : Kirigami.Units.smallSpacing
    readonly property bool textInputActive: textInputFocused()

    spacing: controlSpacing

    function clampValue(value, minimum, maximum) {
        return Math.max(minimum, Math.min(maximum, value));
    }

    function textInputFocused() {
        return zoomSpinBox.activeFocus || (zoomSpinBox.contentItem !== null && zoomSpinBox.contentItem.activeFocus);
    }

    Controls.SpinBox {
        id: zoomSpinBox

        editable: true
        enabled: root.imageReady
        from: Math.min(root.minimumManualZoomPercent, Math.floor(root.imageDocument.zoomPercent))
        implicitWidth: Kirigami.Units.gridUnit * 5
        stepSize: root.zoomStepPercent
        to: Math.max(root.maximumManualZoomPercent, Math.ceil(root.imageDocument.zoomPercent))
        value: Math.round(root.imageDocument.zoomPercent)

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

        onValueModified: root.imageDocument.zoomPercent = root.clampValue(value, root.minimumManualZoomPercent, root.maximumManualZoomPercent)
    }
}
