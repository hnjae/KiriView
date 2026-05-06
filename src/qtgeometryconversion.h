// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_QTGEOMETRYCONVERSION_H
#define KIRIVIEW_QTGEOMETRYCONVERSION_H

#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QtGlobal>

namespace KiriView::Bridge {
template <typename RustSize> RustSize rustSize(const QSize &size)
{
    return RustSize { size.width(), size.height() };
}

template <typename RustSizeF> RustSizeF rustSizeF(const QSizeF &size)
{
    return RustSizeF { size.width(), size.height() };
}

template <typename RustPointF> RustPointF rustPointF(const QPointF &point)
{
    return RustPointF { point.x(), point.y() };
}

template <typename RustRectF> RustRectF rustRectF(const QRectF &rect)
{
    return RustRectF { rect.x(), rect.y(), rect.width(), rect.height() };
}

template <typename RustSize> QSize qtSize(const RustSize &size)
{
    return QSize(size.width, size.height);
}

template <typename RustSizeF> QSizeF qtSizeF(const RustSizeF &size)
{
    return QSizeF(size.width, size.height);
}

inline QSizeF qtSizeF(qreal width, qreal height) { return QSizeF(width, height); }

template <typename RustPointF> QPointF qtPointF(const RustPointF &point)
{
    return QPointF(point.x, point.y);
}

template <typename RustRectF> QRectF qtRectF(const RustRectF &rect)
{
    return QRectF(rect.x, rect.y, rect.width, rect.height);
}
}

#endif
