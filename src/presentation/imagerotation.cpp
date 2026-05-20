// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagerotation.h"

#include "kiriview/src/policy/imagerotation.cxx.h"
#include "qtgeometryconversion.h"

namespace KiriView {
int normalizedImageRotationDegrees(int degrees)
{
    return rustNormalizedImageRotationDegrees(degrees);
}

int imageRotationClockwise(int degrees) { return rustImageRotationClockwise(degrees); }

int imageRotationCounterclockwise(int degrees)
{
    return rustImageRotationCounterclockwise(degrees);
}

bool imageRotationSwapsAxes(int degrees) { return rustImageRotationSwapsAxes(degrees); }

QSize rotatedImageSize(const QSize &size, int degrees)
{
    return Bridge::qtSize(
        rustRotatedImageSize(Bridge::rustSize<RustImageRotationSize>(size), degrees));
}

QSizeF rotatedImageSize(const QSizeF &size, int degrees)
{
    return Bridge::qtSizeF(
        rustRotatedImageSizeF(Bridge::rustSizeF<RustImageRotationSizeF>(size), degrees));
}

QRectF rotatedSourceRectInTarget(
    const QRectF &sourceRect, const QSizeF &sourceSize, const QRectF &targetRect, int degrees)
{
    return Bridge::qtRectF(
        rustRotatedSourceRectInTarget(Bridge::rustRectF<RustImageRotationRectF>(sourceRect),
            Bridge::rustSizeF<RustImageRotationSizeF>(sourceSize),
            Bridge::rustRectF<RustImageRotationRectF>(targetRect), degrees));
}

QRectF unrotatedVisibleRectForRotation(
    const QSizeF &sourceDisplaySize, const QRectF &visibleItemRect, int degrees)
{
    return Bridge::qtRectF(rustUnrotatedVisibleRectForRotation(
        Bridge::rustSizeF<RustImageRotationSizeF>(sourceDisplaySize),
        Bridge::rustRectF<RustImageRotationRectF>(visibleItemRect), degrees));
}

ImageTextureCoordinateTransform imageTextureCoordinateTransform(
    const QRectF &textureRect, int degrees)
{
    const RustImageTextureCoordinateTransform transform = rustImageTextureCoordinateTransform(
        Bridge::rustRectF<RustImageRotationRectF>(textureRect), degrees);
    return ImageTextureCoordinateTransform {
        Bridge::qtPointF(transform.origin),
        Bridge::qtPointF(transform.x_axis),
        Bridge::qtPointF(transform.y_axis),
    };
}
}
