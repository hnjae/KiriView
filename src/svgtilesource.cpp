// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilesource.h"

#include "imagetilesource_p.h"
#include "imageviewtext.h"
#include "kiriview/src/svgrenderer.cxx.h"

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

namespace {
QImage imageFromPremultipliedRgbaBytes(const QByteArray &bytes, const QSize &size)
{
    if (bytes.isEmpty() || size.isEmpty()) {
        return {};
    }

    const std::int64_t expectedSize
        = static_cast<std::int64_t>(size.width()) * static_cast<std::int64_t>(size.height()) * 4;
    if (expectedSize <= 0 || expectedSize > std::numeric_limits<qsizetype>::max()
        || bytes.size() != static_cast<qsizetype>(expectedSize)) {
        return {};
    }

    const QImage image(reinterpret_cast<const uchar *>(bytes.constData()), size.width(),
        size.height(), QImage::Format_RGBA8888_Premultiplied);
    return image.copy();
}
}

namespace KiriView {
std::shared_ptr<SvgTileSource> SvgTileSource::open(const QByteArray &data, QString *errorString)
{
    const RustSvgImageSize intrinsicSize = rustSvgIntrinsicSize(data);
    if (!intrinsicSize.valid) {
        return {};
    }

    const QSize imageSize(intrinsicSize.width, intrinsicSize.height);
    if (imageSize.isEmpty()) {
        setTileSourceError(
            errorString, imageViewText("Could not determine the selected SVG image size."));
        return {};
    }

    return std::make_shared<SvgTileSource>(data, imageSize);
}

SvgTileSource::SvgTileSource(QByteArray data, QSize imageSize)
    : m_data(std::move(data))
    , m_imageSize(std::move(imageSize))
{
}

QSize SvgTileSource::imageSize() const { return m_imageSize; }

std::optional<DecodedTile> SvgTileSource::decodeTile(
    const TileRequest &request, QString *errorString) const
{
    if (!tileRequestCanDecode(request)) {
        return std::nullopt;
    }

    const QByteArray bytes = rustRenderSvgTile(m_data, request.levelSize.width(),
        request.levelSize.height(), request.textureLevelRect.x(), request.textureLevelRect.y(),
        request.textureLevelRect.width(), request.textureLevelRect.height());
    QImage image = imageFromPremultipliedRgbaBytes(bytes, request.textureLevelRect.size());
    if (image.isNull()) {
        setTileSourceError(errorString, imageViewText("Could not render the selected SVG tile."));
        return std::nullopt;
    }

    return decodedTileFromImage(request, std::move(image));
}

QImage SvgTileSource::decodeBlockingDisplayImage(int maximumLongEdge, QString *errorString) const
{
    const QSize previewSize = boundedPreviewSize(m_imageSize, maximumLongEdge);
    const QByteArray bytes = rustRenderSvgImage(m_data, previewSize.width(), previewSize.height());
    const QImage preview = imageFromPremultipliedRgbaBytes(bytes, previewSize);
    if (preview.isNull()) {
        setTileSourceError(errorString, imageViewText("Could not render the selected SVG image."));
        return {};
    }
    return preview;
}

qsizetype SvgTileSource::byteCost() const { return m_data.size(); }
}
