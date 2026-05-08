// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagerectmapping.h"

#include "kiriview/src/imagetilegeometry.cxx.h"

namespace {
KiriView::RustTileSize rustTileSize(const QSize &size)
{
    return KiriView::RustTileSize { size.width(), size.height() };
}

KiriView::RustTileSizeF rustTileSizeF(const QSizeF &size)
{
    return KiriView::RustTileSizeF { size.width(), size.height() };
}

KiriView::RustTileRect rustTileRect(const QRect &rect)
{
    return KiriView::RustTileRect { rect.x(), rect.y(), rect.width(), rect.height() };
}

KiriView::RustTileRectF rustTileRectF(const QRectF &rect)
{
    return KiriView::RustTileRectF { rect.x(), rect.y(), rect.width(), rect.height() };
}

QRect qtRect(const KiriView::RustTileRect &rect)
{
    return QRect(rect.x, rect.y, rect.width, rect.height);
}
}

namespace KiriView {
QRect boundedIntegerRect(const QRect &rect, const QSize &boundsSize)
{
    return qtRect(rustBoundedIntegerRect(rustTileRect(rect), rustTileSize(boundsSize)));
}

QRect scaledIntegerRect(const QRectF &rect, const QSizeF &sourceSize, const QSize &targetSize)
{
    return qtRect(rustScaledIntegerRect(
        rustTileRectF(rect), rustTileSizeF(sourceSize), rustTileSize(targetSize)));
}
}
