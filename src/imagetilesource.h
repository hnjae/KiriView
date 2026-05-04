// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGETILESOURCE_H
#define KIRIVIEW_IMAGETILESOURCE_H

#include "imagesurface.h"

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
    int levelCount() const override;
    QSize levelSize(int level) const override;
    std::optional<DecodedTile> decodeTile(
        const TileRequest &request, QString *errorString) const override;
    QImage decodePreview(int maximumLongEdge, QString *errorString) const override;
    qsizetype byteCost() const override;

private:
    QImage readScaledImage(const QSize &scaledSize, QString *errorString) const;
    QImage readFullImage(QString *errorString) const;
    QImage readSourceClip(const QRect &sourceRect, QString *errorString) const;
    std::optional<QImage> cachedScaledLevel(int level) const;
    void cacheScaledLevel(int level, const QImage &image) const;

    QByteArray m_data;
    QByteArray m_format;
    QSize m_imageSize;
    bool m_hasTransform = false;
    TilePyramid m_pyramid;
    mutable QMutex m_scaledLevelCacheMutex;
    mutable std::vector<std::pair<int, QImage>> m_scaledLevelCache;
};

class SvgTileSource final : public ImageTileSource
{
public:
    static std::shared_ptr<SvgTileSource> open(const QByteArray &data, QString *errorString);

    SvgTileSource(QByteArray data, QSize imageSize);

    QSize imageSize() const override;
    int levelCount() const override;
    QSize levelSize(int level) const override;
    std::optional<DecodedTile> decodeTile(
        const TileRequest &request, QString *errorString) const override;
    QImage decodePreview(int maximumLongEdge, QString *errorString) const override;
    qsizetype byteCost() const override;

private:
    QByteArray m_data;
    QSize m_imageSize;
    TilePyramid m_pyramid;
};

std::shared_ptr<ImageTileSource> openHeifTileSource(const QByteArray &data, QString *errorString);
}

#endif
