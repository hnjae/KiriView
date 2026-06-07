// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQml

QtObject {
    property bool imageHorizontallyPannable: false
    property bool imagePannable: false
    property real viewportHeight: 0
    property real viewportWidth: 0

    function panBy(deltaX, deltaY) {
        return false;
    }

    function panToBottomRight() {
        return false;
    }

    function panToTopLeft() {
        return false;
    }

    function setNextDisplayedImageStartToFinalScanPosition() {
    }

    function zoomByStep(stepCount, viewportX, viewportY) {
        return false;
    }

    function zoomByStepAtCenter(stepCount) {
        return zoomByStep(stepCount, viewportWidth / 2, viewportHeight / 2);
    }
}
