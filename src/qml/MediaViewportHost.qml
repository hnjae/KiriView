// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

Item {
    id: root

    required property KiriDocumentSession documentSession
    required property var openAction
    readonly property KiriImageDocument imageDocument: documentSession.imageDocument
    readonly property bool imageMode: documentSession.documentKind === KiriDocumentSession.Image
    readonly property bool videoMode: documentSession.documentKind === KiriDocumentSession.Video
    readonly property bool imageReady: imageMode && imageDocument.status === KiriImageDocument.Ready && !imageDocument.unsupportedOpenedCollectionVideo
    readonly property url activeDelegateSource: imageMode ? Qt.resolvedUrl("ImageViewport.qml") : videoMode ? Qt.resolvedUrl("VideoViewport.qml") : ""
    readonly property var activeDelegate: mediaViewportDelegateLoader.item
    readonly property ImageViewportInteractionSurface imageInteractionSurface: imageMode && activeDelegate !== null && activeDelegate.imageInteractionSurface !== null ? activeDelegate.imageInteractionSurface : inactiveImageInteractionSurface

    signal viewerClicked
    signal viewerContextMenuRequested(var popupParent, point position)

    function forceActiveViewportFocus() {
        if (root.activeDelegate !== null) {
            root.activeDelegate.requestViewportFocus();
        }
    }

    objectName: "mediaViewportSlot"
    clip: true
    Controls.SplitView.fillHeight: true
    Controls.SplitView.minimumHeight: Kirigami.Units.gridUnit * 6

    ImageViewportInteractionSurface {
        id: inactiveImageInteractionSurface
    }

    Loader {
        id: mediaViewportDelegateLoader

        anchors.fill: parent
        active: root.imageMode || root.videoMode

        function unloadActiveDelegate() {
            if (root.activeDelegate !== null) {
                root.activeDelegate.presentationActive = false;
            }
            if (source.toString().length > 0) {
                source = "";
            }
        }

        function ensureActiveDelegateLoaded() {
            if (!active || root.activeDelegateSource.toString().length === 0) {
                unloadActiveDelegate();
                return;
            }

            if (source.toString() === root.activeDelegateSource.toString()) {
                return;
            }

            unloadActiveDelegate();
            setSource(root.activeDelegateSource, {
                "documentSession": root.documentSession,
                "objectName": root.imageMode ? "imageViewport" : "videoViewport",
                "presentationActive": true
            });
        }

        Component.onCompleted: ensureActiveDelegateLoaded()
        onActiveChanged: ensureActiveDelegateLoaded()
    }

    onActiveDelegateSourceChanged: mediaViewportDelegateLoader.ensureActiveDelegateLoaded()

    Connections {
        target: root.activeDelegate

        function onViewerClicked() {
            root.viewerClicked();
        }

        function onViewerContextMenuRequested(popupParent, position) {
            root.viewerContextMenuRequested(popupParent, position);
        }
    }

    ImageStateOverlay {
        anchors.fill: parent
        imageDocument: root.imageDocument
        imageReady: root.imageReady
        openAction: root.openAction
        visible: !root.videoMode
    }
}
