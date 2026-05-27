// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick

FocusScope {
    id: root

    required property var documentSession
    property bool presentationActive: true
    property MediaViewportInteractionSurface interactionSurface: defaultInteractionSurface
    readonly property int documentKind: root.documentSession !== null && root.documentSession.documentKind !== undefined ? root.documentSession.documentKind : -1

    signal viewerClicked
    signal viewerContextMenuRequested(var popupParent, point position)

    function requestViewportFocus() {
        root.forceActiveFocus();
    }

    activeFocusOnTab: true
    focus: true

    MediaViewportInteractionSurface {
        id: defaultInteractionSurface
    }

    TapHandler {
        id: contextMenuTapHandler

        acceptedButtons: Qt.RightButton
        enabled: root.presentationActive

        onTapped: root.viewerContextMenuRequested(root, contextMenuTapHandler.point.position)
    }
}
