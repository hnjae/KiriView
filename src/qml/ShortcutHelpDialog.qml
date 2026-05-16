// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import io.github.hnjae.kiriview
import org.kde.ki18n
import org.kde.kirigami as Kirigami

Kirigami.OverlaySheet {
    id: root

    required property KiriViewApplication application

    readonly property int shortcutRevision: application.shortcutRevision
    readonly property var helpEntries: [
        {
            "keyText": root.shortcutText(KiriViewApplication.FileOpenAction),
            "description": KI18n.i18nc("@info:whatsthis", "Open an image or comic book")
        },
        {
            "keyText": KI18n.i18nc("@info:keyboard shortcut", "Esc"),
            "description": KI18n.i18nc("@info:whatsthis", "Close dialog or leave fullscreen")
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.FileQuitAction),
            "description": KI18n.i18nc("@info:whatsthis", "Quit KiriView")
        },
        {
            "keyText": root.shortcutTexts([KiriViewApplication.GoPreviousImageAction, KiriViewApplication.GoNextImageAction]),
            "description": KI18n.i18nc("@info:whatsthis", "Previous or next image")
        },
        {
            "keyText": root.shortcutTexts([KiriViewApplication.GoFirstImageAction, KiriViewApplication.GoLastImageAction]),
            "description": KI18n.i18nc("@info:whatsthis", "First or last image")
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.ViewToggleTwoPageModeAction),
            "description": KI18n.i18nc("@info:whatsthis", "Toggle two-page spread")
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.ViewToggleRightToLeftReadingAction),
            "description": KI18n.i18nc("@info:whatsthis", "Toggle right-to-left reading")
        },
        {
            "keyText": root.shortcutTexts([KiriViewApplication.GoPreviousSinglePageAction, KiriViewApplication.GoNextSinglePageAction]),
            "description": KI18n.i18nc("@info:whatsthis", "Adjust two-page spread by one page")
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.ViewZoomInAction),
            "description": KI18n.i18nc("@info:whatsthis", "Zoom in")
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.ViewZoomOutAction),
            "description": KI18n.i18nc("@info:whatsthis", "Zoom out")
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.ViewFitAction),
            "description": KI18n.i18nc("@info:whatsthis", "Fit image")
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.ViewFitHeightAction),
            "description": KI18n.i18nc("@info:whatsthis", "Fit image height")
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.ViewFitWidthAction),
            "description": KI18n.i18nc("@info:whatsthis", "Fit image width")
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.ViewActualSizeAction),
            "description": KI18n.i18nc("@info:whatsthis", "Set 100% zoom")
        },
        {
            "keyText": KI18n.i18nc("@info:keyboard shortcut", "Ctrl+wheel up/down"),
            "description": KI18n.i18nc("@info:whatsthis", "Zoom around the cursor")
        },
        {
            "keyText": root.shortcutTexts([KiriViewApplication.ViewPanLeftAction, KiriViewApplication.ViewPanRightAction]),
            "description": KI18n.i18nc("@info:whatsthis", "Pan horizontally or go previous/next image")
        },
        {
            "keyText": KI18n.i18nc("@info:keyboard shortcut", "Drag / %1", root.shortcutTexts([KiriViewApplication.ViewPanUpAction, KiriViewApplication.ViewPanDownAction])),
            "description": KI18n.i18nc("@info:whatsthis", "Pan the zoomed image")
        },
        {
            "keyText": root.shortcutTexts([KiriViewApplication.ViewPanTopLeftAction, KiriViewApplication.ViewPanBottomRightAction]),
            "description": KI18n.i18nc("@info:whatsthis", "Jump to image pan boundary")
        },
        {
            "keyText": KI18n.i18nc("@info:keyboard shortcut", "Shift+wheel"),
            "description": KI18n.i18nc("@info:whatsthis", "Pan the zoomed image horizontally")
        },
        {
            "keyText": root.shortcutTexts([KiriViewApplication.ViewScanForwardAction, KiriViewApplication.ViewScanBackwardAction]),
            "description": KI18n.i18nc("@info:whatsthis", "Scan image or go previous/next")
        },
        {
            "keyText": root.shortcutTexts([KiriViewApplication.GoPreviousArchiveAction, KiriViewApplication.GoNextArchiveAction]),
            "description": KI18n.i18nc("@info:whatsthis", "Previous or next archive")
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.WindowFullscreenAction),
            "description": KI18n.i18nc("@info:whatsthis", "Toggle fullscreen")
        },
        {
            "keyText": root.shortcutText(KiriViewApplication.HelpShortcutsAction),
            "description": KI18n.i18nc("@info:whatsthis", "Show this shortcut help")
        }
    ]

    showCloseButton: true
    title: KI18n.i18nc("@title:window", "Keyboard Shortcuts")

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
