// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick

ImageToolBar {
    compact: true
    maximumManualZoomPercent: imageDocument.maximumManualZoomPercent
    minimumManualZoomPercent: imageDocument.minimumManualZoomPercent
    zoomEditable: imageReady && imageDocument.zoomPercentKnown
    zoomPercent: imageDocument.zoomPercentKnown ? imageDocument.zoomPercent : 0
    zoomPercentAvailable: imageDocument.zoomPercentKnown
    zoomPercentKnown: imageDocument.zoomPercentKnown
    zoomStepFactor: imageDocument.zoomStepFactor
}
