// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagerectmapping.h"

#include "bridge/qtgeometryconversion.h"
#include "kiriview/src/policy/imagetilegeometry.cxx.h"

namespace {
KiriView::RustTileSize rustTileSize(const QSize &size)
{
    return KiriView::Bridge::rustSize<KiriView::RustTileSize>(size);
}

KiriView::RustTileSizeF rustTileSizeF(const QSizeF &size)
{
    return KiriView::Bridge::rustSizeF<KiriView::RustTileSizeF>(size);
}

KiriView::RustTileRect rustTileRect(const QRect &rect)
{
    return KiriView::Bridge::rustRect<KiriView::RustTileRect>(rect);
}

KiriView::RustTileRectF rustTileRectF(const QRectF &rect)
{
    return KiriView::Bridge::rustRectF<KiriView::RustTileRectF>(rect);
}
}

namespace KiriView {
QRect boundedIntegerRect(const QRect &rect, const QSize &boundsSize)
{
    return Bridge::qtRect(rustBoundedIntegerRect(rustTileRect(rect), rustTileSize(boundsSize)));
}

QRect scaledIntegerRect(const QRectF &rect, const QSizeF &sourceSize, const QSize &targetSize)
{
    return Bridge::qtRect(rustScaledIntegerRect(
        rustTileRectF(rect), rustTileSizeF(sourceSize), rustTileSize(targetSize)));
}
}
