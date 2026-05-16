// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// This wraps Controls.MenuItem because standard menu items cannot render the
// canonical shortcut column without also registering those shortcuts. The
// accessKeysActive hook is paired with MenuAccessKeyRouter so toolbar and
// menubar menus expose the same mnemonic underline behavior.
Controls.MenuItem {
    id: root

    property bool accessKeysActive: false
    readonly property bool menuHasCheckables: root.boolProperty(root.ListView.view, "hasCheckables")
    readonly property bool menuHasIcons: root.boolProperty(root.ListView.view, "hasIcons")
    readonly property string menuShortcutText: root.stringProperty(root.action, "menuShortcutText")

    Kirigami.MnemonicData.active: root.accessKeysActive
    Kirigami.MnemonicData.controlType: Kirigami.MnemonicData.MenuItem
    Kirigami.MnemonicData.enabled: root.enabled && root.visible
    Kirigami.MnemonicData.label: root.text

    function boolProperty(object, name) {
        return object !== null && object !== undefined && name in object ? Boolean(object[name]) : false;
    }

    function stringProperty(object, name) {
        if (object === null || object === undefined || !(name in object)) {
            return "";
        }

        const value = object[name];
        return typeof value === "string" ? value : "";
    }

    function escapedRichText(value) {
        return String(value).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    }

    function richMnemonicText(value, active) {
        let result = "";
        for (let index = 0; index < value.length; ++index) {
            const character = value[index];
            if (character !== "&" || index + 1 >= value.length) {
                result += root.escapedRichText(character);
                continue;
            }

            const mnemonic = value[index + 1];
            if (mnemonic === "&") {
                result += "&amp;";
            } else {
                const escapedMnemonic = root.escapedRichText(mnemonic);
                result += active ? "<u>" + escapedMnemonic + "</u>" : escapedMnemonic;
            }
            ++index;
        }
        return result;
    }

    contentItem: RowLayout {
        Item {
            Layout.preferredWidth: root.menuHasCheckables || root.checkable ? root.indicator.width : Kirigami.Units.smallSpacing
        }

        Kirigami.Icon {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: Kirigami.Settings.hasTransientTouchInput ? Kirigami.Units.iconSizes.smallMedium : Kirigami.Units.iconSizes.small
            Layout.preferredWidth: Layout.preferredHeight
            color: root.icon.color
            selected: root.pressed
            source: root.icon.name !== "" ? root.icon.name : root.icon.source
            visible: root.menuHasIcons || root.icon.name !== "" || root.icon.source.toString() !== ""
        }

        Controls.Label {
            id: label

            objectName: "menuActionItemTextLabel"

            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: true
            color: root.pressed ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
            elide: Text.ElideRight
            font: root.font
            horizontalAlignment: Text.AlignLeft
            text: root.richMnemonicText(root.text, root.accessKeysActive)
            textFormat: Text.RichText
            verticalAlignment: Text.AlignVCenter
            visible: root.text.length > 0
        }

        Controls.Label {
            objectName: "menuActionItemShortcutLabel"

            Layout.alignment: Qt.AlignVCenter
            color: root.pressed ? Kirigami.Theme.highlightedTextColor : Qt.alpha(Kirigami.Theme.textColor, 0.64)
            font: root.font
            horizontalAlignment: Text.AlignRight
            text: root.menuShortcutText
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            visible: root.menuShortcutText.length > 0
        }

        Item {
            Layout.preferredWidth: Kirigami.Units.smallSpacing
        }
    }
}
