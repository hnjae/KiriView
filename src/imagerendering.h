// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGERENDERING_H
#define KIRIVIEW_IMAGERENDERING_H

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QSizeF>
#include <QtGlobal>

namespace KiriView {
inline constexpr int fallbackTextureSizeMax = 16384;

QImage displayReadyImage(const QImage &image);
QSize svgRasterSize(const QSizeF &displaySize, qreal devicePixelRatio, int maximumTextureSize);
QImage renderSvgImage(const QByteArray &data, const QSize &size);
}

#endif
