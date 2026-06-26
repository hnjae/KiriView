// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEVIEWPORTFRAME_H
#define KIRIVIEW_IMAGEVIEWPORTFRAME_H

#include <QPointF>
#include <QRectF>
#include <QSizeF>

namespace kiriview {
struct ImageViewportFrame
{
    QRectF imageRect;
    QSizeF contentSize;
    QPointF maximumContentPosition;
    QPointF contentPosition;
    QRectF visibleItemRect;
    bool horizontalPannable = false;
    bool verticalPannable = false;
    bool pannable = false;

    friend bool operator==(const ImageViewportFrame& left, const ImageViewportFrame& right)
    {
        return left.imageRect == right.imageRect && left.contentSize == right.contentSize
            && left.maximumContentPosition == right.maximumContentPosition
            && left.contentPosition == right.contentPosition
            && left.visibleItemRect == right.visibleItemRect
            && left.horizontalPannable == right.horizontalPannable
            && left.verticalPannable == right.verticalPannable && left.pannable == right.pannable;
    }

    friend bool operator!=(const ImageViewportFrame& left, const ImageViewportFrame& right)
    {
        return !(left == right);
    }
};

ImageViewportFrame projectImageViewportFrame(
    QSizeF viewportSize, QSizeF displaySize, QPointF contentPosition);
}

#endif
