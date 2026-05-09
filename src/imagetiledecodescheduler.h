// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGETILEDECODESCHEDULER_H
#define KIRIVIEW_IMAGETILEDECODESCHEDULER_H

#include "imageasyncticket.h"
#include "imagedocumenttypes.h"
#include "imagesurface.h"

#include <QRectF>
#include <QSet>
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
        const ImageDocumentRenderContext &context);

private:
    struct DecodeLifetime;

    bool tileRequestIsCurrent(quint64 generation, const TileKey &key) const;
    void finishTileDecode(quint64 generation, TileKey key, std::optional<DecodedTile> tile);

    QObject *m_context = nullptr;
    TileDecodedCallback m_tileDecoded;
    std::shared_ptr<DecodeLifetime> m_decodeLifetime;
    ImageAsyncTicket m_generation;
    QSet<TileKey> m_pendingTileKeys;
    QSet<TileKey> m_failedTileKeys;
};
}

#endif
