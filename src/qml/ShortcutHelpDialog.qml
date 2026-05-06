// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

Kirigami.OverlaySheet {
    id: root

    required property KiriViewApplication application

    readonly property int shortcutRevision: application.shortcutRevision
    readonly property var helpEntries: [
        {
            "keyText": root.shortcutText(KiriViewApplication.FileOpenAction),
            "description": "Open an image or comic book"
        },
        {
            "keyText": "Esc",
            "description": "Close dialog or leave fullscreen"
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.FileQuitAction),
            "description": "Quit KiriView"
        },
        {
            "keyText": root.shortcutTexts([KiriViewApplication.GoPreviousImageAction, KiriViewApplication.GoNextImageAction]),
            "description": "Previous or next image"
        },
        {
            "keyText": root.shortcutTexts([KiriViewApplication.GoFirstImageAction, KiriViewApplication.GoLastImageAction]),
            "description": "First or last image"
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.ViewZoomInAction),
            "description": "Zoom in"
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.ViewZoomOutAction),
            "description": "Zoom out"
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.ViewFitAction),
            "description": "Fit image"
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.ViewFitHeightAction),
            "description": "Fit image height"
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.ViewFitWidthAction),
            "description": "Fit image width"
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.ViewActualSizeAction),
            "description": "Set 100% zoom"
        },
        {
            "keyText": "Ctrl+drag up/down",
            "description": "Zoom around the cursor"
        },
        {
            "keyText": "Drag / " + root.shortcutTexts([KiriViewApplication.ViewPanLeftAction, KiriViewApplication.ViewPanRightAction, KiriViewApplication.ViewPanUpAction, KiriViewApplication.ViewPanDownAction]),
            "description": "Pan the zoomed image"
        },
        {
            "keyText": root.shortcutTexts([KiriViewApplication.ViewPanTopLeftAction, KiriViewApplication.ViewPanBottomRightAction]),
            "description": "Jump to image pan boundary"
        },
        {
            "keyText": "Shift+wheel",
            "description": "Pan the zoomed image horizontally"
        },
        {
            "keyText": root.shortcutTexts([KiriViewApplication.ViewScanForwardAction, KiriViewApplication.ViewScanBackwardAction]),
            "description": "Scan image or go previous/next"
        },
        {
            "keyText": root.shortcutTexts([KiriViewApplication.GoPreviousArchiveAction, KiriViewApplication.GoNextArchiveAction]),
            "description": "Previous or next archive"
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.WindowFullscreenAction),
            "description": "Toggle fullscreen"
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.HelpShortcutsAction),
            "description": "Show this shortcut help"
        }
    ]

    showCloseButton: true
    title: "Keyboard Shortcuts"

    function shortcutText(actionId) {
        root.shortcutRevision;
        return root.application.shortcutTextForId(actionId);
    }

    function shortcutTexts(actionIds) {
        root.shortcutRevision;
        return actionIds.map(actionId => root.application.shortcutTextForId(actionId)).filter(text => text.length > 0).join(" / ");
    }

    ColumnLayout {
        Layout.preferredWidth: Kirigami.Units.gridUnit * 28
        spacing: Kirigami.Units.smallSpacing

        Repeater {
            model: root.helpEntries

            delegate: RowLayout {
                id: shortcutRow

                required property var modelData

                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing

                Controls.Label {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 9
                    font.family: "monospace"
                    text: shortcutRow.modelData.keyText
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }

                Controls.Label {
                    Layout.fillWidth: true
                    text: shortcutRow.modelData.description
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}
