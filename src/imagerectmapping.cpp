// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagerectmapping.h"

#include "rendering/imagetilegeometrybridge.h"

namespace KiriView {
QRect boundedIntegerRect(const QRect &rect, const QSize &boundsSize)
{
    return ImageTileGeometryBridge::boundedIntegerRect(rect, boundsSize);
}

QRect scaledIntegerRect(const QRectF &rect, const QSizeF &sourceSize, const QSize &targetSize)
{
    return ImageTileGeometryBridge::scaledIntegerRect(rect, sourceSize, targetSize);
}
}
