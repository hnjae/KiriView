// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagerectmapping.h"

#include "bridge/qtgeometryconversion.h"
#include "kiriview/src/policy/imagetilegeometry.cxx.h"

namespace {
kiriview::RustTileSize rustTileSize(const QSize& size)
{
    return kiriview::Bridge::rustSize<kiriview::RustTileSize>(size);
}

kiriview::RustTileSizeF rustTileSizeF(const QSizeF& size)
{
    return kiriview::Bridge::rustSizeF<kiriview::RustTileSizeF>(size);
}

kiriview::RustTileRect rustTileRect(const QRect& rect)
{
    return kiriview::Bridge::rustRect<kiriview::RustTileRect>(rect);
}

kiriview::RustTileRectF rustTileRectF(const QRectF& rect)
{
    return kiriview::Bridge::rustRectF<kiriview::RustTileRectF>(rect);
}
}

namespace kiriview {
QRect boundedIntegerRect(const QRect& rect, const QSize& boundsSize)
{
    return Bridge::qtRect(rustBoundedIntegerRect(rustTileRect(rect), rustTileSize(boundsSize)));
}

QRect scaledIntegerRect(const QRectF& rect, const QSizeF& sourceSize, const QSize& targetSize)
{
    return Bridge::qtRect(rustScaledIntegerRect(
        rustTileRectF(rect), rustTileSizeF(sourceSize), rustTileSize(targetSize)));
}
}
