// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import io.github.hnjae.kiriview

QtObject {
    id: root

    required property KiriImageView imageView
    required property bool imageReady
    required property bool helpDialogOpen

    signal openDialogRequested

    property Controls.Action openAction: Controls.Action {
        enabled: !root.helpDialogOpen
        icon.name: "document-open-symbolic"
        shortcut: StandardKey.Open
        text: "Open"

        onTriggered: root.openDialogRequested()
    }

    property Controls.Action previousContainerAction: Controls.Action {
        enabled: root.imageView.containerNavigationAvailable && !root.helpDialogOpen
        icon.name: "go-previous-symbolic"
        text: "Previous Container"

        onTriggered: root.imageView.openPreviousContainer()
    }

    property Controls.Action nextContainerAction: Controls.Action {
        enabled: root.imageView.containerNavigationAvailable && !root.helpDialogOpen
        icon.name: "go-next-symbolic"
        text: "Next Container"

        onTriggered: root.imageView.openNextContainer()
    }

    property Controls.Action previousImageAction: Controls.Action {
        enabled: root.imageReady && !root.helpDialogOpen
        icon.name: "go-up-symbolic"
        text: "Previous"

        onTriggered: root.imageView.openPreviousImage()
    }

    property Controls.Action nextImageAction: Controls.Action {
        enabled: root.imageReady && !root.helpDialogOpen
        icon.name: "go-down-symbolic"
        text: "Next"

        onTriggered: root.imageView.openNextImage()
    }

    property Controls.Action fitAction: Controls.Action {
        enabled: root.imageReady && !root.helpDialogOpen
        icon.name: "zoom-fit-best-symbolic"
        text: "Fit"

        onTriggered: root.imageView.resetZoom()
    }

    property Controls.Action fitHeightAction: Controls.Action {
        enabled: root.imageReady && !root.helpDialogOpen
        text: "Fit Height"

        onTriggered: root.imageView.setFitMode(KiriImageView.FitHeight)
    }

    property Controls.Action fitWidthAction: Controls.Action {
        enabled: root.imageReady && !root.helpDialogOpen
        text: "Fit Width"

        onTriggered: root.imageView.setFitMode(KiriImageView.FitWidth)
    }
}
