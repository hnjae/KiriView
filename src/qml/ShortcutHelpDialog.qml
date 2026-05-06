// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

Controls.Dialog {
    id: root

    required property KiriViewApplication application
    required property real windowWidth
    required property real windowHeight

    readonly property int shortcutRevision: application.shortcutRevision
    readonly property var helpEntries: [
        {
            "keyText": root.shortcutText("file_open"),
            "description": "Open an image or comic book"
        },
        {
            "keyText": "Esc",
            "description": "Close dialog or KiriView"
        },
        {
            "keyText": root.shortcutTexts(["go_previous_image", "go_next_image"]),
            "description": "Previous or next image"
        },
        {
            "keyText": root.shortcutTexts(["go_first_image", "go_last_image"]),
            "description": "First or last image"
        },
        {
            "keyText": root.shortcutText("view_zoom_in"),
            "description": "Zoom in"
        },
        {
            "keyText": root.shortcutText("view_zoom_out"),
            "description": "Zoom out"
        },
        {
            "keyText": root.shortcutText("view_fit"),
            "description": "Fit image"
        },
        {
            "keyText": root.shortcutText("view_fit_height"),
            "description": "Fit image height"
        },
        {
            "keyText": root.shortcutText("view_fit_width"),
            "description": "Fit image width"
        },
        {
            "keyText": root.shortcutText("view_actual_size"),
            "description": "Set 100% zoom"
        },
        {
            "keyText": "Ctrl+drag up/down",
            "description": "Zoom around the cursor"
        },
        {
            "keyText": "Drag / " + root.shortcutTexts(["view_pan_left", "view_pan_right", "view_pan_up", "view_pan_down"]),
            "description": "Pan the zoomed image"
        },
        {
            "keyText": root.shortcutTexts(["view_pan_top_left", "view_pan_bottom_right"]),
            "description": "Jump to image pan boundary"
        },
        {
            "keyText": "Shift+wheel",
            "description": "Pan the zoomed image horizontally"
        },
        {
            "keyText": root.shortcutTexts(["view_scan_forward", "view_scan_backward"]),
            "description": "Scan image or go previous/next"
        },
        {
            "keyText": root.shortcutTexts(["go_previous_archive", "go_next_archive"]),
            "description": "Previous or next archive"
        },
        {
            "keyText": root.shortcutText("window_fullscreen"),
            "description": "Toggle fullscreen"
        },
        {
            "keyText": root.shortcutText("help_shortcuts"),
            "description": "Show this shortcut help"
        }
    ]

    closePolicy: Controls.Popup.CloseOnEscape
    modal: true
    standardButtons: Controls.Dialog.Close
    title: "Keyboard Shortcuts"
    width: Math.min(windowWidth - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 28)
    x: Math.round((windowWidth - width) / 2)
    y: Math.round((windowHeight - height) / 2)

    function shortcutText(actionName) {
        root.shortcutRevision;
        return root.application.shortcutText(actionName);
    }

    function shortcutTexts(actionNames) {
        root.shortcutRevision;
        return actionNames.map(actionName => root.application.shortcutText(actionName)).filter(text => text.length > 0).join(" / ");
    }

    contentItem: ColumnLayout {
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
