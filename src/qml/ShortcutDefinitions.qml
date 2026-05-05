// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick

QtObject {
    id: root

    readonly property var openSequence: StandardKey.Open
    readonly property string closeSequence: "Esc"
    readonly property var previousPageSequence: StandardKey.MoveToPreviousPage
    readonly property var nextPageSequence: StandardKey.MoveToNextPage
    readonly property var firstImageSequences: ["Ctrl+Home", "Home"]
    readonly property var lastImageSequences: ["Ctrl+End", "End"]
    readonly property var zoomInControlSequences: ["Ctrl+=", "Ctrl++"]
    readonly property string zoomOutControlSequence: "Ctrl+-"
    readonly property var zoomInCommandSequences: ["=", "+"]
    readonly property string zoomOutCommandSequence: "-"
    readonly property string fitImageSequence: "1"
    readonly property string fitHeightSequence: "2"
    readonly property string fitWidthSequence: "3"
    readonly property string actualSizeSequence: "0"
    readonly property string panLeftSequence: "Left"
    readonly property string panRightSequence: "Right"
    readonly property string panUpSequence: "Up"
    readonly property string panDownSequence: "Down"
    readonly property string panTopLeftSequence: "<"
    readonly property string panBottomRightSequence: ">"
    readonly property string scanForwardSequence: "."
    readonly property string scanBackwardSequence: ","
    readonly property string previousContainerSequence: "["
    readonly property string nextContainerSequence: "]"
    readonly property string fullscreenCommandSequence: "F"
    readonly property string fullscreenFunctionSequence: "F11"
    readonly property string helpCommandSequence: "?"
    readonly property string helpFunctionSequence: "F1"

    readonly property string openKeyText: "Ctrl+O"
    readonly property string closeKeyText: closeSequence
    readonly property string pageNavigationKeyText: "Page Up / Page Down"
    readonly property string firstLastImageKeyText: "Home / End / Ctrl+Home / Ctrl+End"
    readonly property string zoomInKeyText: root.sequenceText(root.zoomInCommandSequences.concat(root.zoomInControlSequences))
    readonly property string zoomOutKeyText: root.sequenceText([root.zoomOutCommandSequence, root.zoomOutControlSequence])
    readonly property string panKeyText: "Drag / Arrow keys"
    readonly property string panBoundaryKeyText: root.sequenceText([root.panTopLeftSequence, root.panBottomRightSequence])
    readonly property string scanKeyText: root.sequenceText([root.scanForwardSequence, root.scanBackwardSequence])
    readonly property string containerNavigationKeyText: root.sequenceText([root.previousContainerSequence, root.nextContainerSequence])
    readonly property string fullscreenKeyText: root.sequenceText([root.fullscreenCommandSequence, root.fullscreenFunctionSequence])
    readonly property string helpKeyText: root.sequenceText([root.helpCommandSequence, root.helpFunctionSequence])

    readonly property var helpEntries: [
        {
            "keyText": root.openKeyText,
            "description": "Open an image or comic book"
        },
        {
            "keyText": root.closeKeyText,
            "description": "Close dialog or KiriView"
        },
        {
            "keyText": root.pageNavigationKeyText,
            "description": "Previous or next image"
        },
        {
            "keyText": root.firstLastImageKeyText,
            "description": "First or last image"
        },
        {
            "keyText": root.zoomInKeyText,
            "description": "Zoom in"
        },
        {
            "keyText": root.zoomOutKeyText,
            "description": "Zoom out"
        },
        {
            "keyText": root.fitImageSequence,
            "description": "Fit image"
        },
        {
            "keyText": root.fitHeightSequence,
            "description": "Fit image height"
        },
        {
            "keyText": root.fitWidthSequence,
            "description": "Fit image width"
        },
        {
            "keyText": root.actualSizeSequence,
            "description": "Set 100% zoom"
        },
        {
            "keyText": "Ctrl+drag up/down",
            "description": "Zoom around the cursor"
        },
        {
            "keyText": root.panKeyText,
            "description": "Pan the zoomed image"
        },
        {
            "keyText": root.panBoundaryKeyText,
            "description": "Jump to image pan boundary"
        },
        {
            "keyText": "Shift+wheel",
            "description": "Pan the zoomed image horizontally"
        },
        {
            "keyText": root.scanKeyText,
            "description": "Scan image or go previous/next"
        },
        {
            "keyText": root.containerNavigationKeyText,
            "description": "Previous or next archive"
        },
        {
            "keyText": root.fullscreenKeyText,
            "description": "Toggle fullscreen"
        },
        {
            "keyText": root.helpKeyText,
            "description": "Show this shortcut help"
        }
    ]

    function sequenceText(sequences) {
        return sequences.join(" / ");
    }
}
