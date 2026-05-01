// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

Controls.ToolBar {
    id: root

    required property KiriImageView imageView
    required property bool imageReady
    required property bool helpDialogOpen
    required property int minimumManualZoomPercent
    required property int maximumManualZoomPercent
    required property int zoomStepPercent
    required property var openAction
    required property var previousContainerAction
    required property var nextContainerAction
    required property var previousImageAction
    required property var nextImageAction
    required property var fitAction
    required property var fitHeightAction
    required property var fitWidthAction

    function clampValue(value, minimum, maximum) {
        return Math.max(minimum, Math.min(maximum, value));
    }

    function pageNumberText() {
        return imageView.currentPageNumber > 0 ? imageView.currentPageNumber.toString() : "0";
    }

    function resetPageNumberText() {
        pageNumberField.resetPageNumberText();
    }

    function textInputFocused() {
        return pageNumberField.activeFocus || zoomSpinBox.activeFocus || (zoomSpinBox.contentItem !== null && zoomSpinBox.contentItem.activeFocus);
    }

    contentItem: Item {
        implicitHeight: Math.max(leftActions.implicitHeight, pageNavigation.implicitHeight, rightActions.implicitHeight) + Kirigami.Units.smallSpacing * 2

        RowLayout {
            id: leftActions

            anchors.left: parent.left
            anchors.leftMargin: Kirigami.Units.smallSpacing
            anchors.verticalCenter: parent.verticalCenter
            spacing: Kirigami.Units.smallSpacing

            Controls.ToolButton {
                action: root.openAction
                display: Controls.AbstractButton.IconOnly

                Controls.ToolTip.text: action.text
                Controls.ToolTip.visible: hovered
            }

            Controls.ToolButton {
                action: root.previousContainerAction
                display: Controls.AbstractButton.IconOnly

                Controls.ToolTip.text: action.text
                Controls.ToolTip.visible: hovered
            }

            Controls.ToolButton {
                action: root.nextContainerAction
                display: Controls.AbstractButton.IconOnly

                Controls.ToolTip.text: action.text
                Controls.ToolTip.visible: hovered
            }
        }

        RowLayout {
            id: pageNavigation

            anchors.centerIn: parent
            enabled: root.imageReady
            spacing: Kirigami.Units.smallSpacing

            Controls.ToolButton {
                action: root.previousImageAction
                display: Controls.AbstractButton.IconOnly

                Controls.ToolTip.text: action.text
                Controls.ToolTip.visible: hovered
            }

            Controls.TextField {
                id: pageNumberField

                Layout.preferredWidth: Math.max(Kirigami.Units.gridUnit * 3, pageNumberMetrics.advanceWidth + leftPadding + rightPadding + Kirigami.Units.smallSpacing * 2)
                enabled: root.imageReady && root.imageView.imageCount > 0
                horizontalAlignment: Text.AlignHCenter
                inputMethodHints: Qt.ImhDigitsOnly
                selectByMouse: true
                validator: IntValidator {
                    bottom: root.imageView.imageCount > 0 ? 1 : 0
                    top: Math.max(1, root.imageView.imageCount)
                }

                function commitPageNumber() {
                    if (!enabled) {
                        resetPageNumberText();
                        return;
                    }

                    const pageNumber = Number(text.trim());
                    if (Number.isInteger(pageNumber) && pageNumber >= 1 && pageNumber <= root.imageView.imageCount) {
                        root.imageView.openImageAtPage(pageNumber);
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
                text: Array(Math.max(1, Math.max(1, root.imageView.imageCount).toString().length) + 1).join("8")
            }

            Controls.Label {
                text: "of"
                textFormat: Text.PlainText
            }

            Controls.Label {
                text: root.imageView.imageCount.toString()
                textFormat: Text.PlainText
            }

            Controls.ToolButton {
                action: root.nextImageAction
                display: Controls.AbstractButton.IconOnly

                Controls.ToolTip.text: action.text
                Controls.ToolTip.visible: hovered
            }
        }

        RowLayout {
            id: rightActions

            anchors.right: parent.right
            anchors.rightMargin: Kirigami.Units.smallSpacing
            anchors.verticalCenter: parent.verticalCenter
            spacing: Kirigami.Units.smallSpacing

            Controls.ToolButton {
                id: fitMenuButton

                enabled: root.imageReady && !root.helpDialogOpen
                display: Controls.AbstractButton.IconOnly
                icon.name: "zoom-fit-best-symbolic"
                text: "Fit"

                onClicked: fitMenu.open()

                Controls.ToolTip.text: text
                Controls.ToolTip.visible: hovered

                Controls.Menu {
                    id: fitMenu

                    y: fitMenuButton.height

                    Controls.MenuItem {
                        action: root.fitAction
                        checkable: true
                        checked: root.imageView.zoomMode === KiriImageView.Fit
                    }

                    Controls.MenuItem {
                        action: root.fitHeightAction
                        checkable: true
                        checked: root.imageView.zoomMode === KiriImageView.FitHeight
                    }

                    Controls.MenuItem {
                        action: root.fitWidthAction
                        checkable: true
                        checked: root.imageView.zoomMode === KiriImageView.FitWidth
                    }
                }
            }

            Controls.SpinBox {
                id: zoomSpinBox

                editable: true
                enabled: root.imageReady
                from: Math.min(root.minimumManualZoomPercent, Math.floor(root.imageView.zoomPercent))
                implicitWidth: Kirigami.Units.gridUnit * 5
                stepSize: root.zoomStepPercent
                to: Math.max(root.maximumManualZoomPercent, Math.ceil(root.imageView.zoomPercent))
                value: Math.round(root.imageView.zoomPercent)

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

                onValueModified: root.imageView.zoomPercent = root.clampValue(value, root.minimumManualZoomPercent, root.maximumManualZoomPercent)
            }
        }
    }

    Connections {
        target: root.imageView

        function onPageNavigationChanged() {
            if (!pageNumberField.activeFocus) {
                pageNumberField.resetPageNumberText();
            }
        }
    }
}
