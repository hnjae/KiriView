// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGERECTMAPPING_H
#define KIRIVIEW_IMAGERECTMAPPING_H

#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>

namespace kiriview {
QRect boundedIntegerRect(QRect rect, QSize boundsSize);
QRect scaledIntegerRect(const QRectF& rect, QSizeF sourceSize, QSize targetSize);
}

#endif
