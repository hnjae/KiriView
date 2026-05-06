// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: root

    required property KiriViewApplication application

    title: "Interface"

    ColumnLayout {
        width: parent.width
        spacing: Kirigami.Units.largeSpacing

        Controls.GroupBox {
            Layout.fillWidth: true
            title: "Application Menu"

            ColumnLayout {
                anchors.fill: parent

                Controls.RadioButton {
                    text: "Hamburger Menu"
                    checked: root.application.menuPresentation === KiriViewApplication.HamburgerMenu

                    onClicked: root.application.menuPresentation = KiriViewApplication.HamburgerMenu
                }

                Controls.RadioButton {
                    text: "Menubar"
                    checked: root.application.menuPresentation === KiriViewApplication.MenuBar

                    onClicked: root.application.menuPresentation = KiriViewApplication.MenuBar
                }
            }
        }
    }
}
