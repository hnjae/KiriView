// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEOZOOMSTATE_H
#define KIRIVIEW_VIDEOZOOMSTATE_H

#include <QRectF>
#include <QtGlobal>
#include <optional>

namespace kiriview {
std::optional<int> videoZoomPercentForRects(
    const QRectF &contentRect, const QRectF &sourceRect, qreal devicePixelRatio);
}

#endif
