// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGETILESOURCE_H
#define KIRIVIEW_IMAGETILESOURCE_H

#include "staticimage.h"

#include <QByteArray>
#include <QImage>
#include <QMutex>
#include <QSize>
#include <QString>
#include <memory>
#include <optional>

namespace KiriView {
class QImageReaderTileSource final : public ImageTileSource
{
public:
    static std::shared_ptr<QImageReaderTileSource> open(
        const QByteArray &data, QString *errorString);

    QImageReaderTileSource(QByteArray data, QByteArray format, QSize imageSize, bool hasTransform);

    QSize imageSize() const override;
    std::optional<DecodedTile> decodeTile(
        const TileRequest &request, QString *errorString) const override;
    FirstDisplayImageDecodeResult decodeFirstDisplayImage(
        const ImageFirstDisplayDecodeContext &context, QString *errorString) const override;
    QImage decodeBlockingDisplayImage(int maximumLongEdge, QString *errorString) const override;
    qsizetype byteCost() const override;

private:
    bool supportsJpegScaledFirstDisplay() const;
    QSize firstDisplayScaledSize(const QSize &physicalViewportSize) const;
    std::optional<DecodedTile> decodeReaderClipTile(
        const TileRequest &request, QString *errorString) const;
    std::optional<DecodedTile> decodeCachedOrScaledLevelTile(
        const TileRequest &request, QString *errorString) const;
    std::optional<DecodedTile> decodeFullImageFallbackTile(
        const TileRequest &request, QString *errorString) const;
    QImage readScaledImage(const QSize &scaledSize, QString *errorString) const;
    QImage readFullImage(QString *errorString) const;
    QImage readSourceClip(const QRect &sourceRect, QString *errorString) const;
    std::optional<QImage> cachedScaledLevel(int level) const;
    void cacheScaledLevel(int level, const QImage &image) const;

    QByteArray m_data;
    QByteArray m_format;
    QSize m_imageSize;
    bool m_hasTransform = false;
    mutable QMutex m_scaledLevelCacheMutex;
    mutable std::vector<std::pair<int, QImage>> m_scaledLevelCache;
};

class SvgTileSource final : public ImageTileSource
{
public:
    static std::shared_ptr<SvgTileSource> open(const QByteArray &data, QString *errorString);

    SvgTileSource(QByteArray data, QSize imageSize);

    QSize imageSize() const override;
    std::optional<DecodedTile> decodeTile(
        const TileRequest &request, QString *errorString) const override;
    FirstDisplayImageDecodeResult decodeFirstDisplayImage(
        const ImageFirstDisplayDecodeContext &context, QString *errorString) const override;
    QImage decodeBlockingDisplayImage(int maximumLongEdge, QString *errorString) const override;
    qsizetype byteCost() const override;

private:
    QByteArray m_data;
    QSize m_imageSize;
};

std::shared_ptr<ImageTileSource> openHeifTileSource(const QByteArray &data, QString *errorString);
}

#endif
