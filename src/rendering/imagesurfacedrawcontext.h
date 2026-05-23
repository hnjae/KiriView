// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESURFACEDRAWCONTEXT_H
#define KIRIVIEW_IMAGESURFACEDRAWCONTEXT_H

#include <QRectF>
#include <QSizeF>
#include <QtGlobal>

namespace KiriView {
struct ImageSurfaceDrawContext {
    QRectF targetRect;
    QSizeF displaySize;
    QRectF visibleItemRect;
    qreal devicePixelRatio = 1.0;
    int rotationDegrees = 0;

    friend bool operator==(
        const ImageSurfaceDrawContext &left, const ImageSurfaceDrawContext &right)
    {
        return left.targetRect == right.targetRect && left.displaySize == right.displaySize
            && left.visibleItemRect == right.visibleItemRect
            && left.devicePixelRatio == right.devicePixelRatio
            && left.rotationDegrees == right.rotationDegrees;
    }

    friend bool operator!=(
        const ImageSurfaceDrawContext &left, const ImageSurfaceDrawContext &right)
    {
        return !(left == right);
    }
};
}

#endif
