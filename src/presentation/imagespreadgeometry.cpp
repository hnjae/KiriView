// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadgeometry.h"

#include "bridge/qtgeometryconversion.h"
#include "kiriview/src/policy/imagespreadgeometry.cxx.h"

namespace kiriview {
QSize imageSpreadImageSize(const QSize& primarySize, const QSize& secondarySize)
{
    return Bridge::qtSize(
        rustImageSpreadImageSize(Bridge::rustSize<RustImageSpreadSize>(primarySize),
            Bridge::rustSize<RustImageSpreadSize>(secondarySize)));
}

QSizeF imageSpreadScaledPageDisplaySize(
    const QSize& pageSize, const QSize& spreadImageSize, const QSizeF& spreadDisplaySize)
{
    return Bridge::qtSizeF(
        rustImageSpreadScaledPageDisplaySize(Bridge::rustSize<RustImageSpreadSize>(pageSize),
            Bridge::rustSize<RustImageSpreadSize>(spreadImageSize),
            Bridge::rustSizeF<RustImageSpreadSizeF>(spreadDisplaySize)));
}

QRectF imageSpreadPrimaryPageRect(const QSizeF& primaryDisplaySize,
    const QSizeF& secondaryDisplaySize, const QSizeF& spreadDisplaySize, bool rightToLeftReading)
{
    return Bridge::qtRectF(
        rustImageSpreadPrimaryPageRect(Bridge::rustSizeF<RustImageSpreadSizeF>(primaryDisplaySize),
            Bridge::rustSizeF<RustImageSpreadSizeF>(secondaryDisplaySize),
            Bridge::rustSizeF<RustImageSpreadSizeF>(spreadDisplaySize), rightToLeftReading));
}

QRectF imageSpreadSecondaryPageRect(const QSizeF& primaryDisplaySize,
    const QSizeF& secondaryDisplaySize, const QSizeF& spreadDisplaySize, bool rightToLeftReading)
{
    return Bridge::qtRectF(rustImageSpreadSecondaryPageRect(
        Bridge::rustSizeF<RustImageSpreadSizeF>(primaryDisplaySize),
        Bridge::rustSizeF<RustImageSpreadSizeF>(secondaryDisplaySize),
        Bridge::rustSizeF<RustImageSpreadSizeF>(spreadDisplaySize), rightToLeftReading));
}

QRectF imageSpreadVisiblePageRect(const QRectF& visibleRect, const QRectF& pageRect)
{
    return Bridge::qtRectF(
        rustImageSpreadVisiblePageRect(Bridge::rustRectF<RustImageSpreadRectF>(visibleRect),
            Bridge::rustRectF<RustImageSpreadRectF>(pageRect)));
}

bool imageSpreadPageIsWide(const QSize& imageSize)
{
    return rustImageSpreadPageIsWide(Bridge::rustSize<RustImageSpreadSize>(imageSize));
}
}
