// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videozoomstate.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
bool positiveFinite(qreal value) { return std::isfinite(value) && value > 0.0; }
}

namespace kiriview {
std::optional<int> videoZoomPercentForRects(
    const QRectF& contentRect, const QRectF& sourceRect, qreal devicePixelRatio)
{
    if (!positiveFinite(contentRect.width()) || !positiveFinite(contentRect.height())
        || !positiveFinite(sourceRect.width()) || !positiveFinite(sourceRect.height())
        || !positiveFinite(devicePixelRatio)) {
        return std::nullopt;
    }

    const qreal widthZoomPercent
        = contentRect.width() * devicePixelRatio / sourceRect.width() * 100.0;
    const qreal heightZoomPercent
        = contentRect.height() * devicePixelRatio / sourceRect.height() * 100.0;
    const qreal zoomPercent = std::min(widthZoomPercent, heightZoomPercent);
    if (!positiveFinite(zoomPercent)) {
        return std::nullopt;
    }
    if (zoomPercent >= std::numeric_limits<int>::max()) {
        return std::numeric_limits<int>::max();
    }

    return static_cast<int>(std::round(zoomPercent));
}
}
