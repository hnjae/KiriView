// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.ki18n
import org.kde.kirigami as Kirigami

RowLayout {
    id: root

    required property bool activeNavigationAvailable
    required property int activeNavigationCount
    required property int activeNavigationCurrentNumber
    required property bool activeNavigationEditable
    required property bool activeNavigationKnown
    required property var openActiveNavigationAtNumber
    required property var actions
    property bool compact: false
    readonly property int controlSpacing: compact ? Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2)) : Kirigami.Units.smallSpacing
    readonly property int currentItemNumber: activeNavigationKnown ? activeNavigationCurrentNumber : 0
    readonly property int itemCount: activeNavigationKnown ? activeNavigationCount : 0
    property int pageNumberDigitCapacity: 1
    property bool rightToLeftReadingActive: false
    readonly property var leftNavigationAction: root.rightToLeftReadingActive ? root.actions.nextImageAction : root.actions.previousImageAction
    readonly property var rightNavigationAction: root.rightToLeftReadingActive ? root.actions.previousImageAction : root.actions.nextImageAction
    readonly property bool textInputActive: pageNumberField.activeFocus
    readonly property bool pageNavigationAvailable: activeNavigationAvailable && activeNavigationKnown

    signal editingCompleted(bool returnViewerFocus)

    enabled: pageNavigationAvailable && activeNavigationEditable
    spacing: controlSpacing

    function cancelEditing(returnViewerFocus) {
        if (pageNumberField.activeFocus) {
            pageNumberField.cancelEditing(returnViewerFocus);
        }
    }

    function commitEditing(returnViewerFocus) {
        if (pageNumberField.activeFocus) {
            pageNumberField.commitEditing(returnViewerFocus);
        }
    }

    function pageNumberText() {
        return currentItemNumber > 0 ? currentItemNumber.toString() : "0";
    }

    function resetPageNumberText() {
        pageNumberField.resetPageNumberText();
    }

    function triggerNavigationAction(action) {
        if (action !== null && action !== undefined && action.enabled) {
            action.trigger();
        }
    }

    function textInputFocused() {
        return pageNumberField.activeFocus;
    }

    Controls.ToolButton {
        id: leftNavigationButton

        objectName: "leftPageNavigationButton"

        property var navigationAction: root.leftNavigationAction
        readonly property string navigationText: navigationAction?.text ?? ""
        readonly property string toolTipText: navigationText

        Accessible.name: text
        Accessible.role: Accessible.Button
        display: Controls.AbstractButton.IconOnly
        enabled: navigationAction !== null && navigationAction !== undefined && navigationAction.enabled
        icon.name: root.actions.previousImageAction?.icon.name ?? ""
        text: navigationText

        Controls.ToolTip.text: toolTipText
        Controls.ToolTip.visible: hovered && Controls.ToolTip.text.length > 0

        onClicked: root.triggerNavigationAction(navigationAction)
    }

    Controls.TextField {
        id: pageNumberField

        objectName: "pageNumberField"

        property bool completingEdit: false

        Layout.preferredWidth: Math.max(Kirigami.Units.gridUnit * 3, pageNumberMetrics.advanceWidth + leftPadding + rightPadding + root.controlSpacing * 2)
        enabled: root.pageNavigationAvailable && root.activeNavigationEditable
        horizontalAlignment: Text.AlignHCenter
        inputMethodHints: Qt.ImhDigitsOnly
        selectByMouse: true
        validator: IntValidator {
            bottom: root.itemCount > 0 ? 1 : 0
            top: Math.max(1, root.itemCount)
        }

        function cancelEditing(returnViewerFocus) {
            if (completingEdit) {
                return;
            }

            completingEdit = true;
            resetPageNumberText();
            focus = false;
            completingEdit = false;
            if (returnViewerFocus === true) {
                root.editingCompleted(true);
            }
        }

        function clampedPageNumber(pageNumber) {
            return Math.max(1, Math.min(root.itemCount, Math.round(pageNumber)));
        }

        function commitEditing(returnViewerFocus) {
            if (completingEdit) {
                return;
            }

            completingEdit = true;
            commitPageNumber();
            focus = false;
            completingEdit = false;
            if (returnViewerFocus === true) {
                root.editingCompleted(true);
            }
        }

        function commitPageNumber() {
            if (!root.pageNavigationAvailable || !root.activeNavigationEditable) {
                resetPageNumberText();
                return;
            }

            const trimmedText = text.trim();
            if (trimmedText.length === 0) {
                resetPageNumberText();
                return;
            }

            const pageNumber = Number(trimmedText);
            if (Number.isFinite(pageNumber)) {
                const targetNumber = clampedPageNumber(pageNumber);
                if (typeof root.openActiveNavigationAtNumber === "function") {
                    root.openActiveNavigationAtNumber(targetNumber);
                }
            }
            resetPageNumberText();
        }

        function resetPageNumberText() {
            text = root.pageNumberText();
        }

        Component.onCompleted: resetPageNumberText()
        onAccepted: commitEditing(true)
        onEditingFinished: commitEditing(false)
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

    TextMetrics {
        id: pageNumberMetrics

        font: pageNumberField.font
        text: Array(Math.max(1, root.pageNumberDigitCapacity) + 1).join("8")
    }

    Controls.Label {
        text: KI18n.i18nc("@label:page count", "of")
        textFormat: Text.PlainText
    }

    Controls.Label {
        id: pageCountLabel

        objectName: "pageCountLabel"

        Layout.preferredWidth: pageCountMetrics.advanceWidth
        horizontalAlignment: Text.AlignLeft
        text: root.itemCount.toString()
        textFormat: Text.PlainText
    }

    TextMetrics {
        id: pageCountMetrics

        font: pageCountLabel.font
        text: Array(Math.max(1, root.pageNumberDigitCapacity) + 1).join("8")
    }

    Controls.ToolButton {
        id: rightNavigationButton

        objectName: "rightPageNavigationButton"

        property var navigationAction: root.rightNavigationAction
        readonly property string navigationText: navigationAction?.text ?? ""
        readonly property string toolTipText: navigationText

        Accessible.name: text
        Accessible.role: Accessible.Button
        display: Controls.AbstractButton.IconOnly
        enabled: navigationAction !== null && navigationAction !== undefined && navigationAction.enabled
        icon.name: root.actions.nextImageAction?.icon.name ?? ""
        text: navigationText

        Controls.ToolTip.text: toolTipText
        Controls.ToolTip.visible: hovered && Controls.ToolTip.text.length > 0

        onClicked: root.triggerNavigationAction(navigationAction)
    }

    onCurrentItemNumberChanged: {
        if (!pageNumberField.activeFocus) {
            pageNumberField.resetPageNumberText();
        }
    }

    onItemCountChanged: {
        if (root.activeNavigationKnown && root.itemCount > 0) {
            root.pageNumberDigitCapacity = Math.max(root.pageNumberDigitCapacity, root.itemCount.toString().length);
        }
        if (!pageNumberField.activeFocus) {
            pageNumberField.resetPageNumberText();
        }
    }

    onActiveNavigationKnownChanged: {
        if (!pageNumberField.activeFocus) {
            pageNumberField.resetPageNumberText();
        }
    }

    onPageNavigationAvailableChanged: {
        if (!pageNumberField.activeFocus) {
            pageNumberField.resetPageNumberText();
        }
    }
}
