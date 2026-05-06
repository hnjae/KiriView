// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import io.github.hnjae.kiriview
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: root

    required property KiriViewApplication application

    title: "Interface"

    FormCard.FormHeader {
        title: "Application Menu"
    }

    FormCard.FormCard {
        FormCard.FormRadioDelegate {
            id: hamburgerMenuDelegate

            checked: root.application.menuPresentation === KiriViewApplication.HamburgerMenu
            text: "Hamburger Menu"

            onClicked: root.application.menuPresentation = KiriViewApplication.HamburgerMenu
        }

        FormCard.FormDelegateSeparator {
            above: menubarDelegate
            below: hamburgerMenuDelegate
        }

        FormCard.FormRadioDelegate {
            id: menubarDelegate

            checked: root.application.menuPresentation === KiriViewApplication.MenuBar
            text: "Menubar"

            onClicked: root.application.menuPresentation = KiriViewApplication.MenuBar
        }
    }
}
