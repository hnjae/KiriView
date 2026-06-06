// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQml

QtObject {
    id: root

    readonly property int angleDeltaPerStep: 120
    property real stepScale: 1.0

    function stepCount(wheel) {
        const verticalDelta = wheel.pixelDelta.y !== 0 ? wheel.pixelDelta.y : wheel.angleDelta.y;
        return verticalDelta / angleDeltaPerStep * stepScale;
    }
}
