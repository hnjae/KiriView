// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import io.github.hnjae.kiriview
import org.kde.ki18n
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: root

    required property KiriViewApplication application

    title: KI18n.i18nc("@title:window", "Interface")

    FormCard.FormHeader {
        title: KI18n.i18nc("@title:group", "Application Menu")
    }

    FormCard.FormCard {
        FormCard.FormRadioDelegate {
            id: hamburgerMenuDelegate

            checked: root.application.menuPresentation === KiriViewApplication.HamburgerMenu
            text: KI18n.i18nc("@option:radio", "Hamburger Menu")

            onClicked: root.application.menuPresentation = KiriViewApplication.HamburgerMenu
        }

        FormCard.FormDelegateSeparator {
            above: menubarDelegate
            below: hamburgerMenuDelegate
        }

        FormCard.FormRadioDelegate {
            id: menubarDelegate

            checked: root.application.menuPresentation === KiriViewApplication.MenuBar
            text: KI18n.i18nc("@option:radio", "Menubar")

            onClicked: root.application.menuPresentation = KiriViewApplication.MenuBar
        }
    }
}
