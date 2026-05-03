// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagerendering.h"

#include <QPainter>
#include <QRectF>
#include <QSvgRenderer>
#include <Qt>
#include <algorithm>
#include <cmath>

namespace KiriView {
QImage displayReadyImage(const QImage &image)
{
    if (image.format() == QImage::Format_RGBA8888_Premultiplied) {
        return image;
    }
    return image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
}

QSize svgRasterSize(const QSizeF &displaySize, qreal devicePixelRatio, int maximumTextureSize)
{
    if (displaySize.isEmpty() || !std::isfinite(devicePixelRatio) || devicePixelRatio <= 0.0) {
        return {};
    }

    const qreal width = displaySize.width() * devicePixelRatio;
    const qreal height = displaySize.height() * devicePixelRatio;
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0 || height <= 0.0) {
        return {};
    }

    const int maximumSize = maximumTextureSize > 0 ? maximumTextureSize : fallbackTextureSizeMax;
    const qreal scale = std::min<qreal>(1.0, std::min(maximumSize / width, maximumSize / height));
    return QSize(std::clamp(static_cast<int>(std::ceil(width * scale)), 1, maximumSize),
        std::clamp(static_cast<int>(std::ceil(height * scale)), 1, maximumSize));
}

QImage renderSvgImage(const QByteArray &data, const QSize &size)
{
    if (data.isEmpty() || size.isEmpty()) {
        return {};
    }

    QSvgRenderer renderer(data);
    if (!renderer.isValid()) {
        return {};
    }

    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    if (image.isNull()) {
        return {};
    }

    image.fill(Qt::transparent);
    QPainter painter(&image);
    renderer.render(&painter, QRectF(QPointF(0.0, 0.0), QSizeF(size)));
    return image;
}
}
