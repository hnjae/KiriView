// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_SVGTILESOURCE_H
#define KIRIVIEW_SVGTILESOURCE_H

#include "staticimage.h"

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QString>
#include <memory>
#include <optional>

namespace KiriView {
class SvgTileSource final : public ImageTileSource
{
public:
    static std::shared_ptr<SvgTileSource> open(const QByteArray &data, QString *errorString);

    SvgTileSource(QByteArray data, QSize imageSize);

    QSize imageSize() const override;
    std::optional<DecodedTile> decodeTile(
        const TileRequest &request, QString *errorString) const override;
    QImage decodeBlockingDisplayImage(int maximumLongEdge, QString *errorString) const override;
    qsizetype byteCost() const override;

private:
    QByteArray m_data;
    QSize m_imageSize;
};
}

#endif
