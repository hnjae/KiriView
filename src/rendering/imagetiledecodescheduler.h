// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGETILEDECODESCHEDULER_H
#define KIRIVIEW_IMAGETILEDECODESCHEDULER_H

#include "document/imagedocumenttypes.h"
#include "imagesurface.h"
#include "imagetiledecodestate.h"

#include <QRectF>
#include <QSizeF>
#include <functional>
#include <memory>
#include <optional>

class QObject;

namespace KiriView {
class ImageTileDecodeScheduler final
{
public:
    using TileDecodedCallback = std::function<void(DecodedTile)>;

    ImageTileDecodeScheduler(QObject *context, TileDecodedCallback tileDecoded);
    ~ImageTileDecodeScheduler();

    void invalidate();
    void schedule(const std::shared_ptr<DisplayedImageSurface> &displayedSurface,
        const QSizeF &displaySize, const QRectF &visibleItemRect,
        const ImageDocumentRenderContext &context, int rotationDegrees = 0);

private:
    struct DecodeLifetime;

    void finishTileDecode(quint64 generation, TileKey key, std::optional<DecodedTile> tile);

    QObject *m_context = nullptr;
    TileDecodedCallback m_tileDecoded;
    std::shared_ptr<DecodeLifetime> m_decodeLifetime;
    ImageTileDecodeState m_decodeState;
};
}

#endif
