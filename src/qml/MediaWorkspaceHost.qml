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
    required property color viewerForegroundColor
    required property color viewerSurfaceColor
    property bool infoPanelOpen: false
    readonly property real infoPanelMinimumWidth: Kirigami.Units.gridUnit * 16
    readonly property real infoPanelPreferredWidth: Kirigami.Units.gridUnit * 18
    readonly property real infoPanelMaximumWidth: Kirigami.Units.gridUnit * 20
    readonly property real infoPanelWideBreakpoint: Kirigami.Units.gridUnit * 42
    readonly property bool infoPanelInlineMode: width >= infoPanelWideBreakpoint
    readonly property ImageViewportInteractionSurface imageInteractionSurface: mediaViewportHost.imageInteractionSurface
    readonly property bool infoPanelVisible: infoPanelOpen
    readonly property bool thumbnailPanelVisible: thumbnailPanel.visible

    signal viewerClicked
    signal viewerContextMenuRequested(var popupParent, point position)

    function forceActiveViewportFocus() {
        mediaViewportHost.forceActiveViewportFocus();
    }

    function toggleInfoPanel() {
        infoPanelOpen = !infoPanelOpen;
    }

    function closeInfoPanel() {
        infoPanelOpen = false;
    }

    function toggleThumbnailPanel() {
        thumbnailPanel.visible = !thumbnailPanel.visible;
    }

    function syncInfoPanelDrawer() {
        const drawerShouldOpen = infoPanelOpen && !infoPanelInlineMode;
        if (infoPanelOverlayDrawer.drawerOpen === drawerShouldOpen) {
            return;
        }

        infoPanelOverlayDrawer.drawerOpen = drawerShouldOpen;
    }

    onInfoPanelInlineModeChanged: syncInfoPanelDrawer()
    onInfoPanelOpenChanged: syncInfoPanelDrawer()

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
                viewerForegroundColor: root.viewerForegroundColor
                viewerSurfaceColor: root.viewerSurfaceColor
                visible: false
                Controls.SplitView.maximumHeight: Kirigami.Units.gridUnit * 7.5
                Controls.SplitView.minimumHeight: Kirigami.Units.gridUnit * 6
                Controls.SplitView.preferredHeight: Math.min(Kirigami.Units.gridUnit * 7.5, Math.max(Kirigami.Units.gridUnit * 6, mediaPanelSplitView.height * 0.2))
            }
        }

        InfoPanel {
            id: infoPanel

            documentSession: root.documentSession
            objectName: "infoPanel"
            visible: root.infoPanelOpen && root.infoPanelInlineMode
            Controls.SplitView.maximumWidth: root.infoPanelMaximumWidth
            Controls.SplitView.minimumWidth: root.infoPanelMinimumWidth
            Controls.SplitView.preferredWidth: root.infoPanelPreferredWidth

            onCloseRequested: root.closeInfoPanel()
        }
    }

    Kirigami.OverlayDrawer {
        id: infoPanelOverlayDrawer

        objectName: "infoPanelOverlayDrawer"

        closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnReleaseOutside
        edge: Qt.RightEdge
        handleVisible: false
        maximumSize: Math.min(root.infoPanelMaximumWidth, root.width)
        minimumSize: Math.min(root.infoPanelMinimumWidth, maximumSize)
        modal: true
        preferredSize: Math.min(maximumSize, Math.max(minimumSize, root.infoPanelPreferredWidth))

        contentItem: InfoPanel {
            documentSession: root.documentSession
            objectName: "infoPanelOverlayContent"

            onCloseRequested: root.closeInfoPanel()
        }

        onDrawerOpenChanged: {
            if (!drawerOpen && root.infoPanelOpen && !root.infoPanelInlineMode) {
                root.infoPanelOpen = false;
            }
        }
    }
}
