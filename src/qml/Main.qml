// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    title: imageView.windowTitleFileName.length > 0 ? imageView.windowTitleFileName + " — KiriView" : "KiriView"
    visible: true

    property bool helpDialogOpen: false
    property url initialSourceUrl
    property int visibilityBeforeFullscreen: Window.Windowed

    function restoredVisibility(visibility) {
        switch (visibility) {
        case Window.Windowed:
        case Window.Maximized:
        case Window.Minimized:
            return visibility;
        default:
            return Window.Windowed;
        }
    }

    function toggleFullScreen() {
        if (visibility === Window.FullScreen) {
            visibility = restoredVisibility(visibilityBeforeFullscreen);
            return;
        }

        visibilityBeforeFullscreen = restoredVisibility(visibility);
        visibility = Window.FullScreen;
    }

    minimumWidth: Kirigami.Units.gridUnit * 28
    minimumHeight: Kirigami.Units.gridUnit * 20
    width: minimumWidth
    height: minimumHeight

    Shortcut {
        context: Qt.WindowShortcut
        sequence: "Esc"

        onActivated: {
            if (root.helpDialogOpen) {
                shortcutHelpDialog.close();
                return;
            }

            root.close();
        }
    }

    pageStack.initialPage: Kirigami.Page {
        id: page

        background: Rectangle {
            color: "#3c3c3c"
        }

        padding: 0
        globalToolBarStyle: Kirigami.ApplicationHeaderStyle.None

        readonly property bool imageReady: imageView.status === KiriImageView.Ready
        readonly property real dragZoomPercentPerPixel: zoomStepPercent / Math.max(1, Kirigami.Units.gridUnit * 2)
        readonly property bool imagePannable: imageFlickable.contentWidth > imageFlickable.width || imageFlickable.contentHeight > imageFlickable.height
        readonly property int keyboardPanDistance: 64
        readonly property int maximumManualZoomPercent: 800
        readonly property int minimumManualZoomPercent: 10
        readonly property int zoomStepPercent: 10

        function clampValue(value, minimum, maximum) {
            return Math.max(minimum, Math.min(maximum, value));
        }

        function maximumContentX() {
            return Math.max(0, Math.max(imageFlickable.width, imageView.x + imageView.width) - imageFlickable.width);
        }

        function maximumContentY() {
            return Math.max(0, Math.max(imageFlickable.height, imageView.y + imageView.height) - imageFlickable.height);
        }

        function panBy(deltaX, deltaY) {
            if (!imagePannable) {
                return false;
            }

            const nextContentX = clampValue(imageFlickable.contentX + deltaX, 0, maximumContentX());
            const nextContentY = clampValue(imageFlickable.contentY + deltaY, 0, maximumContentY());
            const moved = nextContentX !== imageFlickable.contentX || nextContentY !== imageFlickable.contentY;
            imageFlickable.contentX = nextContentX;
            imageFlickable.contentY = nextContentY;
            return moved;
        }

        function pageNumberText() {
            return imageView.currentPageNumber > 0 ? imageView.currentPageNumber.toString() : "0";
        }

        function textInputFocused() {
            return pageNumberField.activeFocus || zoomSpinBox.activeFocus || (zoomSpinBox.contentItem !== null && zoomSpinBox.contentItem.activeFocus);
        }

        function viewportPointInsideImage(viewportX, viewportY) {
            if (!imageReady || imageView.width <= 0 || imageView.height <= 0) {
                return false;
            }

            const contentPointX = imageFlickable.contentX + viewportX;
            const contentPointY = imageFlickable.contentY + viewportY;
            return contentPointX >= imageView.x && contentPointX <= imageView.x + imageView.width && contentPointY >= imageView.y && contentPointY <= imageView.y + imageView.height;
        }

        function zoomBy(deltaPercent, viewportX, viewportY) {
            if (!imageReady) {
                return false;
            }

            const nextZoomPercent = clampValue(imageView.zoomPercent + deltaPercent, minimumManualZoomPercent, maximumManualZoomPercent);
            if (Math.abs(nextZoomPercent - imageView.zoomPercent) < 0.001) {
                return false;
            }

            const anchorViewportX = Number.isFinite(viewportX) ? clampValue(viewportX, 0, imageFlickable.width) : imageFlickable.width / 2;
            const anchorViewportY = Number.isFinite(viewportY) ? clampValue(viewportY, 0, imageFlickable.height) : imageFlickable.height / 2;
            const anchorRatioX = imageView.width > 0 ? clampValue((imageFlickable.contentX + anchorViewportX - imageView.x) / imageView.width, 0, 1) : 0.5;
            const anchorRatioY = imageView.height > 0 ? clampValue((imageFlickable.contentY + anchorViewportY - imageView.y) / imageView.height, 0, 1) : 0.5;

            imageView.zoomPercent = nextZoomPercent;
            imageFlickable.contentX = clampValue(imageView.x + anchorRatioX * imageView.width - anchorViewportX, 0, maximumContentX());
            imageFlickable.contentY = clampValue(imageView.y + anchorRatioY * imageView.height - anchorViewportY, 0, maximumContentY());
            return true;
        }

        Controls.Action {
            id: openAction

            enabled: !root.helpDialogOpen
            icon.name: "document-open-symbolic"
            shortcut: StandardKey.Open
            text: "Open"

            onTriggered: fileDialog.open()
        }

        Controls.Action {
            id: previousImageAction

            enabled: page.imageReady && !root.helpDialogOpen
            icon.name: "go-up-symbolic"
            text: "Previous"

            onTriggered: imageView.openPreviousImage()
        }

        Controls.Action {
            id: nextImageAction

            enabled: page.imageReady && !root.helpDialogOpen
            icon.name: "go-down-symbolic"
            text: "Next"

            onTriggered: imageView.openNextImage()
        }

        Controls.Action {
            id: previousContainerAction

            enabled: imageView.containerNavigationAvailable && !root.helpDialogOpen
            icon.name: "go-previous-symbolic"
            text: "Previous Container"

            onTriggered: imageView.openPreviousContainer()
        }

        Controls.Action {
            id: nextContainerAction

            enabled: imageView.containerNavigationAvailable && !root.helpDialogOpen
            icon.name: "go-next-symbolic"
            text: "Next Container"

            onTriggered: imageView.openNextContainer()
        }

        Controls.Action {
            id: fitAction

            enabled: page.imageReady && !root.helpDialogOpen
            icon.name: "zoom-fit-best-symbolic"
            text: "Fit"

            onTriggered: imageView.resetZoom()
        }

        Controls.Action {
            id: fitHeightAction

            enabled: page.imageReady && !root.helpDialogOpen
            text: "Fit Height"

            onTriggered: imageView.setFitMode(KiriImageView.FitHeight)
        }

        Controls.Action {
            id: fitWidthAction

            enabled: page.imageReady && !root.helpDialogOpen
            text: "Fit Width"

            onTriggered: imageView.setFitMode(KiriImageView.FitWidth)
        }

        header: Controls.ToolBar {
            contentItem: Item {
                implicitHeight: Math.max(leftActions.implicitHeight, pageNavigation.implicitHeight, rightActions.implicitHeight) + Kirigami.Units.smallSpacing * 2

                RowLayout {
                    id: leftActions

                    anchors.left: parent.left
                    anchors.leftMargin: Kirigami.Units.smallSpacing
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Kirigami.Units.smallSpacing

                    Controls.ToolButton {
                        action: openAction
                        display: Controls.AbstractButton.IconOnly

                        Controls.ToolTip.text: action.text
                        Controls.ToolTip.visible: hovered
                    }

                    Controls.ToolButton {
                        action: previousContainerAction
                        display: Controls.AbstractButton.IconOnly

                        Controls.ToolTip.text: action.text
                        Controls.ToolTip.visible: hovered
                    }

                    Controls.ToolButton {
                        action: nextContainerAction
                        display: Controls.AbstractButton.IconOnly

                        Controls.ToolTip.text: action.text
                        Controls.ToolTip.visible: hovered
                    }
                }

                RowLayout {
                    id: pageNavigation

                    anchors.centerIn: parent
                    enabled: page.imageReady
                    spacing: Kirigami.Units.smallSpacing

                    Controls.ToolButton {
                        action: previousImageAction
                        display: Controls.AbstractButton.IconOnly

                        Controls.ToolTip.text: action.text
                        Controls.ToolTip.visible: hovered
                    }

                    Controls.TextField {
                        id: pageNumberField

                        Layout.preferredWidth: Math.max(Kirigami.Units.gridUnit * 3, pageNumberMetrics.advanceWidth + leftPadding + rightPadding + Kirigami.Units.smallSpacing * 2)
                        enabled: page.imageReady && imageView.imageCount > 0
                        horizontalAlignment: Text.AlignHCenter
                        inputMethodHints: Qt.ImhDigitsOnly
                        selectByMouse: true
                        validator: IntValidator {
                            bottom: imageView.imageCount > 0 ? 1 : 0
                            top: Math.max(1, imageView.imageCount)
                        }

                        function commitPageNumber() {
                            if (!enabled) {
                                resetPageNumberText();
                                return;
                            }

                            const pageNumber = Number(text.trim());
                            if (Number.isInteger(pageNumber) && pageNumber >= 1 && pageNumber <= imageView.imageCount) {
                                imageView.openImageAtPage(pageNumber);
                            }
                            resetPageNumberText();
                        }

                        function resetPageNumberText() {
                            text = page.pageNumberText();
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
                        text: Array(Math.max(1, Math.max(1, imageView.imageCount).toString().length) + 1).join("8")
                    }

                    Controls.Label {
                        text: "of"
                        textFormat: Text.PlainText
                    }

                    Controls.Label {
                        text: imageView.imageCount.toString()
                        textFormat: Text.PlainText
                    }

                    Controls.ToolButton {
                        action: nextImageAction
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

                        enabled: page.imageReady && !root.helpDialogOpen
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
                                action: fitAction
                                checkable: true
                                checked: imageView.zoomMode === KiriImageView.Fit
                            }

                            Controls.MenuItem {
                                action: fitHeightAction
                                checkable: true
                                checked: imageView.zoomMode === KiriImageView.FitHeight
                            }

                            Controls.MenuItem {
                                action: fitWidthAction
                                checkable: true
                                checked: imageView.zoomMode === KiriImageView.FitWidth
                            }
                        }
                    }

                    Controls.SpinBox {
                        id: zoomSpinBox

                        editable: true
                        enabled: page.imageReady
                        from: Math.min(page.minimumManualZoomPercent, Math.floor(imageView.zoomPercent))
                        implicitWidth: Kirigami.Units.gridUnit * 5
                        stepSize: page.zoomStepPercent
                        to: Math.max(page.maximumManualZoomPercent, Math.ceil(imageView.zoomPercent))
                        value: Math.round(imageView.zoomPercent)

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

                        onValueModified: imageView.zoomPercent = page.clampValue(value, page.minimumManualZoomPercent, page.maximumManualZoomPercent)
                    }
                }
            }
        }

        Connections {
            target: imageView

            function onPageNavigationChanged() {
                if (!pageNumberField.activeFocus) {
                    pageNumberField.resetPageNumberText();
                }
            }
        }

        Flickable {
            id: imageFlickable

            anchors.fill: parent
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            contentHeight: Math.max(height, imageView.y + imageView.height)
            contentWidth: Math.max(width, imageView.x + imageView.width)
            interactive: imageView.status === KiriImageView.Ready && (contentWidth > width || contentHeight > height)

            Controls.ScrollBar.horizontal: Controls.ScrollBar {
                policy: imageFlickable.contentWidth > imageFlickable.width ? Controls.ScrollBar.AsNeeded : Controls.ScrollBar.AlwaysOff
            }

            Controls.ScrollBar.vertical: Controls.ScrollBar {
                policy: imageFlickable.contentHeight > imageFlickable.height ? Controls.ScrollBar.AsNeeded : Controls.ScrollBar.AlwaysOff
            }

            KiriImageView {
                id: imageView

                height: displaySize.height
                viewportSize: Qt.size(imageFlickable.width, imageFlickable.height)
                width: displaySize.width
                x: Math.max(0, (imageFlickable.width - width) / 2)
                y: Math.max(0, (imageFlickable.height - height) / 2)

                Component.onCompleted: {
                    if (root.initialSourceUrl.toString().length > 0) {
                        sourceUrl = root.initialSourceUrl;
                    }
                }
            }
        }

        MouseArea {
            id: zoomDragArea

            anchors.fill: imageFlickable
            acceptedButtons: Qt.LeftButton
            enabled: page.imageReady

            property real lastY: 0
            property bool zoomDragActive: false

            onCanceled: zoomDragActive = false
            onPositionChanged: mouse => {
                if (!zoomDragActive) {
                    mouse.accepted = false;
                    return;
                }

                const deltaY = mouse.y - lastY;
                if (deltaY !== 0) {
                    page.zoomBy(-deltaY * page.dragZoomPercentPerPixel, mouse.x, mouse.y);
                    lastY = mouse.y;
                }
                mouse.accepted = true;
            }
            onPressed: mouse => {
                if ((mouse.modifiers & Qt.ControlModifier) && page.viewportPointInsideImage(mouse.x, mouse.y)) {
                    zoomDragActive = true;
                    lastY = mouse.y;
                    mouse.accepted = true;
                    return;
                }

                zoomDragActive = false;
                mouse.accepted = false;
            }
            onReleased: mouse => {
                zoomDragActive = false;
                mouse.accepted = true;
            }
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imageReady && !root.helpDialogOpen
            sequence: "Ctrl+="

            onActivated: page.zoomBy(page.zoomStepPercent, imageFlickable.width / 2, imageFlickable.height / 2)
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imageReady && !root.helpDialogOpen
            sequence: "Ctrl++"

            onActivated: page.zoomBy(page.zoomStepPercent, imageFlickable.width / 2, imageFlickable.height / 2)
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imageReady && !root.helpDialogOpen
            sequence: "Ctrl+-"

            onActivated: page.zoomBy(-page.zoomStepPercent, imageFlickable.width / 2, imageFlickable.height / 2)
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imagePannable && !page.textInputFocused() && !root.helpDialogOpen
            sequence: "Left"

            onActivated: page.panBy(-page.keyboardPanDistance, 0)
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imagePannable && !page.textInputFocused() && !root.helpDialogOpen
            sequence: "Right"

            onActivated: page.panBy(page.keyboardPanDistance, 0)
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imagePannable && !page.textInputFocused() && !root.helpDialogOpen
            sequence: "Up"

            onActivated: page.panBy(0, -page.keyboardPanDistance)
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imagePannable && !page.textInputFocused() && !root.helpDialogOpen
            sequence: "Down"

            onActivated: page.panBy(0, page.keyboardPanDistance)
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imageReady && !root.helpDialogOpen
            sequence: StandardKey.MoveToPreviousPage

            onActivated: imageView.openPreviousImage()
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imageReady && !root.helpDialogOpen
            sequence: StandardKey.MoveToNextPage

            onActivated: imageView.openNextImage()
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: imageView.containerNavigationAvailable && !page.textInputFocused() && !root.helpDialogOpen
            sequence: "["

            onActivated: imageView.openPreviousContainer()
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: imageView.containerNavigationAvailable && !page.textInputFocused() && !root.helpDialogOpen
            sequence: "]"

            onActivated: imageView.openNextContainer()
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: !page.textInputFocused() && !root.helpDialogOpen
            sequence: "F"

            onActivated: root.toggleFullScreen()
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: !root.helpDialogOpen
            sequence: "F11"

            onActivated: root.toggleFullScreen()
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: !page.textInputFocused() && !root.helpDialogOpen
            sequence: "?"

            onActivated: shortcutHelpDialog.open()
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: !root.helpDialogOpen
            sequence: "F1"

            onActivated: shortcutHelpDialog.open()
        }

        Controls.BusyIndicator {
            anchors.centerIn: parent
            running: visible
            visible: imageView.status === KiriImageView.Loading
        }

        Rectangle {
            anchors.margins: Kirigami.Units.largeSpacing
            anchors.right: parent.right
            anchors.top: parent.top
            color: Qt.rgba(0, 0, 0, 0.55)
            height: Kirigami.Units.gridUnit * 2
            radius: Kirigami.Units.smallSpacing
            visible: page.imageReady && imageView.loading
            width: height

            Controls.BusyIndicator {
                anchors.fill: parent
                anchors.margins: Kirigami.Units.smallSpacing
                running: visible
            }
        }

        Kirigami.InlineMessage {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.margins: Kirigami.Units.largeSpacing
            anchors.right: parent.right
            text: imageView.errorString
            type: Kirigami.MessageType.Error
            visible: page.imageReady && !imageView.loading && imageView.errorString.length > 0
        }

        ColumnLayout {
            anchors.centerIn: parent
            spacing: Kirigami.Units.largeSpacing
            visible: imageView.status === KiriImageView.Null
            width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 18)

            Kirigami.Icon {
                Layout.alignment: Qt.AlignHCenter
                implicitHeight: Kirigami.Units.iconSizes.huge
                implicitWidth: Kirigami.Units.iconSizes.huge
                source: "image-x-generic-symbolic"
            }

            Controls.Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: "No image selected"
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
            }

            Controls.Button {
                Layout.alignment: Qt.AlignHCenter
                icon.name: "document-open-symbolic"
                text: "Open"

                onClicked: fileDialog.open()
            }
        }

        ColumnLayout {
            anchors.centerIn: parent
            spacing: Kirigami.Units.largeSpacing
            visible: imageView.status === KiriImageView.Error
            width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 24)

            Kirigami.Icon {
                Layout.alignment: Qt.AlignHCenter
                implicitHeight: Kirigami.Units.iconSizes.huge
                implicitWidth: Kirigami.Units.iconSizes.huge
                source: "dialog-error-symbolic"
            }

            Controls.Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: "Unable to open image"
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
            }

            Controls.Label {
                Layout.fillWidth: true
                color: Kirigami.Theme.disabledTextColor
                horizontalAlignment: Text.AlignHCenter
                text: imageView.errorString
                textFormat: Text.PlainText
                visible: text.length > 0
                wrapMode: Text.Wrap
            }

            Controls.Button {
                Layout.alignment: Qt.AlignHCenter
                icon.name: "document-open-symbolic"
                text: "Open"

                onClicked: fileDialog.open()
            }
        }
    }

    Controls.Dialog {
        id: shortcutHelpDialog

        closePolicy: Controls.Popup.NoAutoClose
        modal: true
        standardButtons: Controls.Dialog.Close
        title: "Keyboard Shortcuts"
        width: Math.min(root.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 28)
        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)

        onClosed: root.helpDialogOpen = false
        onOpened: root.helpDialogOpen = true

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            Repeater {
                model: ListModel {
                    ListElement {
                        description: "Open an image or comic book"
                        keys: "Ctrl+O"
                    }
                    ListElement {
                        description: "Close dialog or KiriView"
                        keys: "Esc"
                    }
                    ListElement {
                        description: "Previous or next image"
                        keys: "Page Up / Page Down"
                    }
                    ListElement {
                        description: "Zoom in"
                        keys: "Ctrl+= / Ctrl++"
                    }
                    ListElement {
                        description: "Zoom out"
                        keys: "Ctrl+-"
                    }
                    ListElement {
                        description: "Zoom around the cursor"
                        keys: "Ctrl+drag up/down"
                    }
                    ListElement {
                        description: "Pan the zoomed image"
                        keys: "Arrow keys"
                    }
                    ListElement {
                        description: "Previous or next container"
                        keys: "[ / ]"
                    }
                    ListElement {
                        description: "Toggle fullscreen"
                        keys: "F / F11"
                    }
                    ListElement {
                        description: "Show this shortcut help"
                        keys: "? / F1"
                    }
                }

                delegate: RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.largeSpacing

                    Controls.Label {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 9
                        font.family: "monospace"
                        text: keys
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                    }

                    Controls.Label {
                        Layout.fillWidth: true
                        text: description
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }

    Dialogs.FileDialog {
        id: fileDialog

        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: ["Image and comic book files (*.avif *.avifs *.bmp *.cbz *.gif *.jpeg *.jpg *.png *.svg *.webp)", "All files (*)"]
        title: "Open Image or Comic Book"

        onAccepted: imageView.sourceUrl = selectedFile
    }
}
