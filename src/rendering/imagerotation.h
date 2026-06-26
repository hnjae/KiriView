// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEROTATION_H
#define KIRIVIEW_IMAGEROTATION_H

#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>

namespace kiriview {
struct ImageTextureCoordinateTransform
{
    QPointF origin;
    QPointF xAxis;
    QPointF yAxis;
};

int normalizedImageRotationDegrees(int degrees);
int imageRotationClockwise(int degrees);
int imageRotationCounterclockwise(int degrees);
bool imageRotationSwapsAxes(int degrees);
QSize rotatedImageSize(const QSize& size, int degrees);
QSizeF rotatedImageSize(const QSizeF& size, int degrees);
QRectF rotatedSourceRectInTarget(
    const QRectF& sourceRect, const QSizeF& sourceSize, const QRectF& targetRect, int degrees);
QRectF unrotatedVisibleRectForRotation(
    const QSizeF& sourceDisplaySize, const QRectF& visibleItemRect, int degrees);
ImageTextureCoordinateTransform imageTextureCoordinateTransform(
    const QRectF& textureRect, int degrees);
}

#endif
