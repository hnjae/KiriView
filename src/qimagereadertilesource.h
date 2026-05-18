// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_QIMAGEREADERTILESOURCE_H
#define KIRIVIEW_QIMAGEREADERTILESOURCE_H

#include "qimagereaderscaledlevelcache.h"
#include "staticimage.h"

#include <QByteArray>
#include <QImage>
#include <QRect>
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
    std::optional<DecodedTile> decodeReaderClipTile(
        const TileRequest &request, QString *errorString) const;
    std::optional<DecodedTile> decodeCachedOrScaledLevelTile(
        const TileRequest &request, QString *errorString) const;
    std::optional<DecodedTile> decodeFullImageFallbackTile(
        const TileRequest &request, QString *errorString) const;
    QImage readScaledImage(const QSize &scaledSize, QString *errorString) const;
    QImage readFullImage(QString *errorString) const;
    QImage readSourceClip(const QRect &sourceRect, QString *errorString) const;

    QByteArray m_data;
    QByteArray m_format;
    QSize m_imageSize;
    bool m_hasTransform = false;
    mutable QImageReaderScaledLevelCache m_scaledLevelCache;
};
}

#endif
