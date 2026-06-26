// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_BRIDGE_QTGEOMETRYCONVERSION_H
#define KIRIVIEW_BRIDGE_QTGEOMETRYCONVERSION_H

#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QtGlobal>

namespace kiriview::Bridge {
template <typename RustSize> RustSize rustSize(QSize size)
{
    return RustSize { size.width(), size.height() };
}

template <typename RustSizeF> RustSizeF rustSizeF(QSizeF size)
{
    return RustSizeF { size.width(), size.height() };
}

template <typename RustPointF> RustPointF rustPointF(QPointF point)
{
    return RustPointF { point.x(), point.y() };
}

template <typename RustRect> RustRect rustRect(QRect rect)
{
    return RustRect { rect.x(), rect.y(), rect.width(), rect.height() };
}

template <typename RustRectF> RustRectF rustRectF(const QRectF& rect)
{
    return RustRectF { rect.x(), rect.y(), rect.width(), rect.height() };
}

template <typename RustSize> QSize qtSize(const RustSize& size)
{
    return QSize(size.width, size.height);
}

template <typename RustSizeF> QSizeF qtSizeF(const RustSizeF& size)
{
    return QSizeF(size.width, size.height);
}

inline QSizeF qtSizeF(qreal width, qreal height) { return QSizeF(width, height); }

template <typename RustPointF> QPointF qtPointF(const RustPointF& point)
{
    return QPointF(point.x, point.y);
}

template <typename RustRect> QRect qtRect(const RustRect& rect)
{
    return QRect(rect.x, rect.y, rect.width, rect.height);
}

template <typename RustRectF> QRectF qtRectF(const RustRectF& rect)
{
    return QRectF(rect.x, rect.y, rect.width, rect.height);
}
}

#endif
