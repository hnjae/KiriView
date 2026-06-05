// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_HEIFTILESOURCE_H
#define KIRIVIEW_HEIFTILESOURCE_H

#include "decoding/heiftilingplan.h"
#include "staticimage.h"

#include <QByteArray>
#include <QRect>
#include <QString>
#include <memory>
#include <optional>

namespace KiriView {
class HeifTileSource final : public ImageTileSource
{
public:
    HeifTileSource(QByteArray data, QSize imageSize, std::optional<HeifTileGrid> tileGrid);

    QSize imageSize() const override;
    qsizetype byteCost() const override;
    bool supportsRasterDisplayRefinement() const override;
    QImage decodeRasterDisplayImage(const QSize &rasterSize, QString *errorString) const override;
    QImage decodeBlockingDisplayImage(int maximumLongEdge, QString *errorString) const override;
    std::optional<DecodedTile> decodeTile(
        const TileRequest &request, QString *errorString) const override;

private:
    QImage decodeFullOrScaled(const QSize &targetSize, QString *errorString) const;
    QImage decodeGridRasterDisplayImage(const QSize &rasterSize, QString *errorString) const;
    QImage decodeGridSourceRect(const QRect &sourceRect, QString *errorString) const;

    QByteArray m_data;
    QSize m_imageSize;
    std::optional<HeifTileGrid> m_tileGrid;
};

std::shared_ptr<HeifTileSource> openHeifTileSource(const QByteArray &data, QString *errorString);
}

#endif
