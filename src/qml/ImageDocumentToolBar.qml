// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick

ImageToolBar {
    compact: true
    maximumManualZoomPercent: imageDocument.maximumManualZoomPercent
    minimumManualZoomPercent: imageDocument.minimumManualZoomPercent
    zoomStepFactor: imageDocument.zoomStepFactor
}
