// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "svgtilesource.h"

#include "bridge/rustqtconversion.h"
#include "imagerendering.h"
#include "imagetilesourcehelpers_p.h"
#include "kiriview/src/policy/svgrenderer.cxx.h"
#include "localization/imageerrortext.h"

#include <QByteArray>
#include <QImage>
#include <QRectF>
#include <QSize>
#include <QtMath>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

namespace {
QImage imageFromPremultipliedRgbaBytes(const QByteArray& bytes, QSize size)
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

    const QImage image(reinterpret_cast<const uchar*>(bytes.constData()), size.width(),
        size.height(), QImage::Format_RGBA8888_Premultiplied);
    return image.copy();
}

QByteArray renderSvgImageBytes(const QByteArray& data, QSize size)
{
    if (size.isEmpty()) {
        return {};
    }

    return kiriview::Bridge::qtByteArray(kiriview::rustRenderSvgImage(
        kiriview::Bridge::rustBytes(data), size.width(), size.height()));
}

QImage renderSvgImage(const QByteArray& data, QSize size)
{
    return imageFromPremultipliedRgbaBytes(renderSvgImageBytes(data, size), size);
}

QSize svgFirstDisplayPreviewSize(QSize imageSize, QSize physicalViewportSize)
{
    if (imageSize.isEmpty() || physicalViewportSize.isEmpty()) {
        return {};
    }

    const QRectF targetRect = kiriview::imageTargetRect(imageSize, QSizeF(physicalViewportSize));
    if (targetRect.isEmpty()) {
        return {};
    }

    return QSize {
        std::clamp(qCeil(targetRect.width()), 1, physicalViewportSize.width()),
        std::clamp(qCeil(targetRect.height()), 1, physicalViewportSize.height()),
    };
}
}

namespace kiriview {
std::shared_ptr<SvgTileSource> SvgTileSource::open(const QByteArray& data, QString* errorString)
{
    const RustSvgImageSize intrinsicSize = rustSvgIntrinsicSize(Bridge::rustBytes(data));
    if (!intrinsicSize.valid) {
        return {};
    }

    const QSize imageSize(intrinsicSize.width, intrinsicSize.height);
    if (imageSize.isEmpty()) {
        setTileSourceError(errorString, imageErrorText(ImageErrorTextId::DetermineSvgImageSize));
        return {};
    }

    return std::make_shared<SvgTileSource>(data, imageSize);
}

SvgTileSource::SvgTileSource(QByteArray data, QSize imageSize)
    : m_data(std::move(data))
    , m_imageSize(imageSize)
{
}

QSize SvgTileSource::imageSize() const { return m_imageSize; }

std::optional<DecodedTile> SvgTileSource::decodeTile(
    const TileRequest& request, QString* errorString) const
{
    if (!tileRequestCanDecode(request)) {
        return std::nullopt;
    }

    const QByteArray bytes = Bridge::qtByteArray(
        rustRenderSvgTile(Bridge::rustBytes(m_data), request.levelSize.width(),
            request.levelSize.height(), request.textureLevelRect.x(), request.textureLevelRect.y(),
            request.textureLevelRect.width(), request.textureLevelRect.height()));
    QImage image = imageFromPremultipliedRgbaBytes(bytes, request.textureLevelRect.size());
    if (image.isNull()) {
        setTileSourceError(errorString, imageErrorText(ImageErrorTextId::RenderSvgTile));
        return std::nullopt;
    }

    return decodedTileFromImage(request, std::move(image));
}

FirstDisplayImageDecodeResult SvgTileSource::decodeFirstDisplayImage(
    const ImageFirstDisplayDecodeContext& context, QString* errorString) const
{
    if (!context.isValid()) {
        return {};
    }

    const QSize previewSize = svgFirstDisplayPreviewSize(m_imageSize, context.physicalViewportSize);
    if (previewSize.isEmpty()) {
        return {};
    }

    QImage preview = renderSvgImage(m_data, previewSize);
    if (preview.isNull()) {
        setTileSourceError(errorString, imageErrorText(ImageErrorTextId::RenderSvgImage));
        return { FirstDisplayImageDecodeStatus::Error, {}, 0.0 };
    }

    const qreal displayPixelsPerSourcePixel
        = imagePixelsPerSourcePixel(m_imageSize, preview.size());
    if (displayPixelsPerSourcePixel <= 0.0) {
        setTileSourceError(errorString, imageErrorText(ImageErrorTextId::RenderSvgImage));
        return { FirstDisplayImageDecodeStatus::Error, {}, 0.0 };
    }

    return { FirstDisplayImageDecodeStatus::Ready, std::move(preview),
        displayPixelsPerSourcePixel };
}

bool SvgTileSource::supportsRasterDisplayRefinement() const { return true; }

QImage SvgTileSource::decodeRasterDisplayImage(const QSize& rasterSize, QString* errorString) const
{
    const QImage image = renderSvgImage(m_data, rasterSize);
    if (image.isNull()) {
        setTileSourceError(errorString, imageErrorText(ImageErrorTextId::RenderSvgImage));
        return {};
    }
    return image;
}

QImage SvgTileSource::decodeBlockingDisplayImage(int maximumLongEdge, QString* errorString) const
{
    const QSize previewSize = boundedPreviewSize(m_imageSize, maximumLongEdge);
    const QImage preview = renderSvgImage(m_data, previewSize);
    if (preview.isNull()) {
        setTileSourceError(errorString, imageErrorText(ImageErrorTextId::RenderSvgImage));
        return {};
    }
    return preview;
}

qsizetype SvgTileSource::byteCost() const { return m_data.size(); }

bool SvgTileSource::isResolutionIndependent() const { return true; }
}
