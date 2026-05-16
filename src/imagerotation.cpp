// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagerotation.h"

#include <QtGlobal>

namespace KiriView {
int normalizedImageRotationDegrees(int degrees)
{
    const int normalized = ((degrees % 360) + 360) % 360;
    return normalized % 90 == 0 ? normalized : 0;
}

int imageRotationClockwise(int degrees) { return normalizedImageRotationDegrees(degrees + 90); }

int imageRotationCounterclockwise(int degrees)
{
    return normalizedImageRotationDegrees(degrees - 90);
}

bool imageRotationSwapsAxes(int degrees)
{
    const int normalized = normalizedImageRotationDegrees(degrees);
    return normalized == 90 || normalized == 270;
}

QSize rotatedImageSize(const QSize &size, int degrees)
{
    if (!imageRotationSwapsAxes(degrees)) {
        return size;
    }

    return QSize(size.height(), size.width());
}

QSizeF rotatedImageSize(const QSizeF &size, int degrees)
{
    if (!imageRotationSwapsAxes(degrees)) {
        return size;
    }

    return QSizeF(size.height(), size.width());
}

QRectF rotatedSourceRectInTarget(
    const QRectF &sourceRect, const QSizeF &sourceSize, const QRectF &targetRect, int degrees)
{
    if (sourceRect.isEmpty() || sourceSize.isEmpty() || targetRect.isEmpty()) {
        return {};
    }

    const qreal sourceWidth = sourceSize.width();
    const qreal sourceHeight = sourceSize.height();
    if (sourceWidth <= 0.0 || sourceHeight <= 0.0) {
        return {};
    }

    qreal x = 0.0;
    qreal y = 0.0;
    qreal width = 0.0;
    qreal height = 0.0;

    switch (normalizedImageRotationDegrees(degrees)) {
    case 90:
        x = (sourceHeight - sourceRect.y() - sourceRect.height()) / sourceHeight;
        y = sourceRect.x() / sourceWidth;
        width = sourceRect.height() / sourceHeight;
        height = sourceRect.width() / sourceWidth;
        break;
    case 180:
        x = (sourceWidth - sourceRect.x() - sourceRect.width()) / sourceWidth;
        y = (sourceHeight - sourceRect.y() - sourceRect.height()) / sourceHeight;
        width = sourceRect.width() / sourceWidth;
        height = sourceRect.height() / sourceHeight;
        break;
    case 270:
        x = sourceRect.y() / sourceHeight;
        y = (sourceWidth - sourceRect.x() - sourceRect.width()) / sourceWidth;
        width = sourceRect.height() / sourceHeight;
        height = sourceRect.width() / sourceWidth;
        break;
    case 0:
    default:
        x = sourceRect.x() / sourceWidth;
        y = sourceRect.y() / sourceHeight;
        width = sourceRect.width() / sourceWidth;
        height = sourceRect.height() / sourceHeight;
        break;
    }

    return QRectF(targetRect.x() + x * targetRect.width(), targetRect.y() + y * targetRect.height(),
        width * targetRect.width(), height * targetRect.height());
}

QRectF unrotatedVisibleRectForRotation(
    const QSizeF &sourceDisplaySize, const QRectF &visibleItemRect, int degrees)
{
    if (sourceDisplaySize.isEmpty() || visibleItemRect.isEmpty()) {
        return {};
    }

    switch (normalizedImageRotationDegrees(degrees)) {
    case 90:
        return QRectF(visibleItemRect.y(),
            sourceDisplaySize.height() - visibleItemRect.x() - visibleItemRect.width(),
            visibleItemRect.height(), visibleItemRect.width());
    case 180:
        return QRectF(sourceDisplaySize.width() - visibleItemRect.x() - visibleItemRect.width(),
            sourceDisplaySize.height() - visibleItemRect.y() - visibleItemRect.height(),
            visibleItemRect.width(), visibleItemRect.height());
    case 270:
        return QRectF(sourceDisplaySize.width() - visibleItemRect.y() - visibleItemRect.height(),
            visibleItemRect.x(), visibleItemRect.height(), visibleItemRect.width());
    case 0:
    default:
        return visibleItemRect;
    }
}

ImageTextureCoordinateTransform imageTextureCoordinateTransform(
    const QRectF &textureRect, int degrees)
{
    const qreal left = textureRect.x();
    const qreal top = textureRect.y();
    const qreal right = textureRect.x() + textureRect.width();
    const qreal bottom = textureRect.y() + textureRect.height();
    const qreal width = textureRect.width();
    const qreal height = textureRect.height();

    switch (normalizedImageRotationDegrees(degrees)) {
    case 90:
        return ImageTextureCoordinateTransform {
            QPointF(left, bottom),
            QPointF(0.0, -height),
            QPointF(width, 0.0),
        };
    case 180:
        return ImageTextureCoordinateTransform {
            QPointF(right, bottom),
            QPointF(-width, 0.0),
            QPointF(0.0, -height),
        };
    case 270:
        return ImageTextureCoordinateTransform {
            QPointF(right, top),
            QPointF(0.0, height),
            QPointF(-width, 0.0),
        };
    case 0:
    default:
        return ImageTextureCoordinateTransform {
            QPointF(left, top),
            QPointF(width, 0.0),
            QPointF(0.0, height),
        };
    }
}
}
