// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

Item {
    id: root

    objectName: "mediaWorkspaceHost"

    required property KiriDocumentSession documentSession
    required property var openAction
    readonly property ImageViewportInteractionSurface imageInteractionSurface: mediaViewportHost.imageInteractionSurface
    readonly property bool infoPanelVisible: infoPanel.visible
    readonly property bool thumbnailPanelVisible: thumbnailPanel.visible

    signal viewerClicked
    signal viewerContextMenuRequested(var popupParent, point position)

    function forceActiveViewportFocus() {
        mediaViewportHost.forceActiveViewportFocus();
    }

    function toggleInfoPanel() {
        infoPanel.visible = !infoPanel.visible;
    }

    function toggleThumbnailPanel() {
        thumbnailPanel.visible = !thumbnailPanel.visible;
    }

    Controls.SplitView {
        id: contentSplitView

        objectName: "contentSplitView"
        anchors.fill: parent
        orientation: Qt.Horizontal

        Controls.SplitView {
            id: mediaPanelSplitView

            objectName: "mediaPanelSplitView"
            orientation: Qt.Vertical
            Controls.SplitView.fillWidth: true
            Controls.SplitView.minimumWidth: Kirigami.Units.gridUnit * 8

            MediaViewportHost {
                id: mediaViewportHost

                documentSession: root.documentSession
                openAction: root.openAction

                onViewerClicked: root.viewerClicked()
                onViewerContextMenuRequested: function (popupParent, position) {
                    root.viewerContextMenuRequested(popupParent, position);
                }
            }

            ThumbnailPanel {
                id: thumbnailPanel

                documentSession: root.documentSession
                objectName: "thumbnailPanel"
                visible: false
                Controls.SplitView.maximumHeight: Kirigami.Units.gridUnit * 12
                Controls.SplitView.minimumHeight: Kirigami.Units.gridUnit * 10
                Controls.SplitView.preferredHeight: Math.min(Kirigami.Units.gridUnit * 12, Math.max(Kirigami.Units.gridUnit * 10, mediaPanelSplitView.height * 0.28))
            }
        }

        InfoPanel {
            id: infoPanel

            objectName: "infoPanel"
            visible: false
            Controls.SplitView.maximumWidth: Kirigami.Units.gridUnit * 18
            Controls.SplitView.minimumWidth: Kirigami.Units.gridUnit * 8
            Controls.SplitView.preferredWidth: Math.min(Kirigami.Units.gridUnit * 18, Math.max(Kirigami.Units.gridUnit * 10, contentSplitView.width * 0.3))
        }
    }
}
