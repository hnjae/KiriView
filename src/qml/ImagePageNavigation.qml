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
    required property var actions
    property bool compact: false
    readonly property int controlSpacing: compact ? Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2)) : Kirigami.Units.smallSpacing
    readonly property bool textInputActive: pageNumberField.activeFocus

    enabled: imageReady
    spacing: controlSpacing

    function pageNumberText() {
        return imageDocument.currentPageNumber > 0 ? imageDocument.currentPageNumber.toString() : "0";
    }

    function resetPageNumberText() {
        pageNumberField.resetPageNumberText();
    }

    function textInputFocused() {
        return pageNumberField.activeFocus;
    }

    Controls.ToolButton {
        action: root.actions.previousImageAction
        display: Controls.AbstractButton.IconOnly

        Controls.ToolTip.text: action.text
        Controls.ToolTip.visible: hovered
    }

    Controls.TextField {
        id: pageNumberField

        Layout.preferredWidth: Math.max(Kirigami.Units.gridUnit * 3, pageNumberMetrics.advanceWidth + leftPadding + rightPadding + root.controlSpacing * 2)
        enabled: root.imageReady && root.imageDocument.imageCount > 0
        horizontalAlignment: Text.AlignHCenter
        inputMethodHints: Qt.ImhDigitsOnly
        selectByMouse: true
        validator: IntValidator {
            bottom: root.imageDocument.imageCount > 0 ? 1 : 0
            top: Math.max(1, root.imageDocument.imageCount)
        }

        function commitPageNumber() {
            if (!enabled) {
                resetPageNumberText();
                return;
            }

            const pageNumber = Number(text.trim());
            if (Number.isInteger(pageNumber) && pageNumber >= 1 && pageNumber <= root.imageDocument.imageCount) {
                root.imageDocument.openImageAtPage(pageNumber);
            }
            resetPageNumberText();
        }

        function resetPageNumberText() {
            text = root.pageNumberText();
        }

        Component.onCompleted: resetPageNumberText()
        onAccepted: {
            commitPageNumber();
            focus = false;
        }
        onEditingFinished: commitPageNumber()
    }

    TextMetrics {
        id: pageNumberMetrics

        font: pageNumberField.font
        text: Array(Math.max(1, Math.max(1, root.imageDocument.imageCount).toString().length) + 1).join("8")
    }

    Controls.Label {
        text: "of"
        textFormat: Text.PlainText
    }

    Controls.Label {
        text: root.imageDocument.imageCount.toString()
        textFormat: Text.PlainText
    }

    Controls.ToolButton {
        action: root.actions.nextImageAction
        display: Controls.AbstractButton.IconOnly

        Controls.ToolTip.text: action.text
        Controls.ToolTip.visible: hovered
    }

    Connections {
        target: root.imageDocument

        function onPageNavigationChanged() {
            if (!pageNumberField.activeFocus) {
                pageNumberField.resetPageNumberText();
            }
        }
    }
}
