// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionactivezoom.h"

#include "session/documentsessionpublicprojection.h"

namespace kiriview {
ActiveZoomSnapshot documentSessionActiveZoomSnapshot(DocumentSessionKind documentKind,
    const DocumentSessionPublicImageLeafSnapshot& image,
    const DocumentSessionPublicVideoLeafSnapshot& video)
{
    switch (documentKind) {
    case DocumentSessionKind::Image:
        if (!image.readyForInformation || !image.zoomPercentKnown) {
            return {};
        }
        return ActiveZoomSnapshot { true, true, image.zoomPercent, true };
    case DocumentSessionKind::Video:
        return ActiveZoomSnapshot {
            true,
            video.zoomPercentKnown,
            video.zoomPercentKnown ? qreal(video.zoomPercent) : 0.0,
            false,
        };
    case DocumentSessionKind::Empty:
        return {};
    }

    return {};
}
}
