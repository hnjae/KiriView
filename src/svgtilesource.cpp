// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilesource.h"

#include "imagerendering.h"
#include "imagetilesource_p.h"
#include "imageviewtext.h"

#include <QPainter>
#include <QRectF>
#include <QSvgRenderer>
#include <memory>
#include <optional>
#include <utility>

namespace KiriView {
std::shared_ptr<SvgTileSource> SvgTileSource::open(const QByteArray &data, QString *errorString)
{
    QSvgRenderer renderer(data);
    if (!renderer.isValid()) {
        return {};
    }

    const QSize intrinsicSize = svgIntrinsicSize(renderer);
    if (intrinsicSize.isEmpty()) {
        setTileSourceError(
            errorString, imageViewText("Could not determine the selected SVG image size."));
        return {};
    }

    return std::make_shared<SvgTileSource>(data, intrinsicSize);
}

SvgTileSource::SvgTileSource(QByteArray data, QSize imageSize)
    : m_data(std::move(data))
    , m_imageSize(std::move(imageSize))
    , m_pyramid(m_imageSize)
{
}

QSize SvgTileSource::imageSize() const { return m_imageSize; }

int SvgTileSource::levelCount() const { return m_pyramid.levelCount(); }

QSize SvgTileSource::levelSize(int level) const { return m_pyramid.levelSize(level); }

std::optional<DecodedTile> SvgTileSource::decodeTile(
    const TileRequest &request, QString *errorString) const
{
    QSvgRenderer renderer(m_data);
    if (!renderer.isValid()) {
        setTileSourceError(errorString, imageViewText("Could not decode the selected SVG image."));
        return std::nullopt;
    }

    QImage image(request.textureLevelRect.size(), QImage::Format_RGBA8888_Premultiplied);
    if (image.isNull()) {
        setTileSourceError(errorString, imageViewText("Could not render the selected SVG tile."));
        return std::nullopt;
    }

    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.scale(static_cast<qreal>(image.width()) / request.textureLevelRect.width(),
        static_cast<qreal>(image.height()) / request.textureLevelRect.height());
    painter.translate(-request.textureLevelRect.x(), -request.textureLevelRect.y());
    renderer.render(&painter, QRectF(QPointF(0.0, 0.0), QSizeF(request.levelSize)));
    return decodedTileFromImage(request, std::move(image));
}

FirstDisplayImageDecodeResult SvgTileSource::decodeFirstDisplayImage(
    const ImageFirstDisplayDecodeContext &context, QString *errorString) const
{
    Q_UNUSED(context);
    Q_UNUSED(errorString);
    // TODO: Add an SVG-specific first-display strategy if rendering cost becomes visible.
    return {};
}

QImage SvgTileSource::decodeBlockingDisplayImage(int maximumLongEdge, QString *errorString) const
{
    const QSize previewSize = boundedPreviewSize(m_imageSize, maximumLongEdge);
    const QImage preview = renderSvgImage(m_data, previewSize);
    if (preview.isNull()) {
        setTileSourceError(errorString, imageViewText("Could not render the selected SVG image."));
        return {};
    }
    return preview;
}

qsizetype SvgTileSource::byteCost() const { return m_data.size(); }
}
