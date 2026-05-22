// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import io.github.hnjae.kiriview
import org.kde.ki18n
import org.kde.kirigami as Kirigami

RowLayout {
    id: root

    required property KiriImageDocument imageDocument
    required property bool imageReady
    required property var actions
    property bool compact: false
    property bool fileDeletionInProgress: imageDocument.fileDeletionInProgress
    property bool mediaNavigationActive: false
    property bool mediaNavigationKnown: false
    property int currentMediaNumber: 0
    property int mediaCount: 0
    readonly property int controlSpacing: compact ? Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2)) : Kirigami.Units.smallSpacing
    readonly property int currentItemNumber: mediaNavigationActive ? currentMediaNumber : imageDocument.currentPageNumber
    readonly property int itemCount: mediaNavigationActive ? mediaCount : imageDocument.imageCount
    property bool rightToLeftReadingActive: false
    readonly property var leftNavigationAction: root.rightToLeftReadingActive ? root.actions.nextImageAction : root.actions.previousImageAction
    readonly property var rightNavigationAction: root.rightToLeftReadingActive ? root.actions.previousImageAction : root.actions.nextImageAction
    readonly property bool textInputActive: pageNumberField.activeFocus
    readonly property bool pageNavigationAvailable: itemCount > 0 && !fileDeletionInProgress && (!mediaNavigationActive || mediaNavigationKnown)

    signal editingCompleted(bool returnViewerFocus)
    signal mediaNumberRequested(int mediaNumber)

    enabled: pageNavigationAvailable
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

        property bool completingEdit: false

        Layout.preferredWidth: Math.max(Kirigami.Units.gridUnit * 3, pageNumberMetrics.advanceWidth + leftPadding + rightPadding + root.controlSpacing * 2)
        enabled: root.pageNavigationAvailable
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
            if (!enabled) {
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
                if (root.mediaNavigationActive) {
                    root.mediaNumberRequested(targetNumber);
                } else {
                    root.imageDocument.openImageAtPage(targetNumber);
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
        text: Array(Math.max(1, Math.max(1, root.itemCount).toString().length) + 1).join("8")
    }

    Controls.Label {
        text: KI18n.i18nc("@label:page count", "of")
        textFormat: Text.PlainText
    }

    Controls.Label {
        text: root.itemCount.toString()
        textFormat: Text.PlainText
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
        if (!pageNumberField.activeFocus) {
            pageNumberField.resetPageNumberText();
        }
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
