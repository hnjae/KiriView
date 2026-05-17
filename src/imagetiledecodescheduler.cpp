// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetiledecodescheduler.h"

#include "imageasyncworker.h"
#include "imagetiledecodeplan.h"

#include <QString>
#include <memory>
#include <optional>
#include <utility>

namespace KiriView {
struct ImageTileDecodeScheduler::DecodeLifetime {
};

ImageTileDecodeScheduler::ImageTileDecodeScheduler(
    QObject *context, TileDecodedCallback tileDecoded)
    : m_context(context)
    , m_tileDecoded(std::move(tileDecoded))
    , m_decodeLifetime(std::make_shared<DecodeLifetime>())
{
}

ImageTileDecodeScheduler::~ImageTileDecodeScheduler() { m_decodeLifetime.reset(); }

void ImageTileDecodeScheduler::invalidate()
{
    m_generation.invalidate();
    m_pendingTileKeys.clear();
    m_failedTileKeys.clear();
}

void ImageTileDecodeScheduler::schedule(
    const std::shared_ptr<DisplayedImageSurface> &displayedSurface, const QSizeF &displaySize,
    const QRectF &visibleItemRect, const ImageDocumentRenderContext &context, int rotationDegrees)
{
    const ImageTileDecodePlan plan = imageTileDecodePlan(displayedSurface, displaySize,
        visibleItemRect, context, rotationDegrees, m_pendingTileKeys, m_failedTileKeys);
    if (plan.isEmpty()) {
        return;
    }
    const quint64 generation = m_generation.current();

    for (const TileRequest &request : plan.requests) {
        m_pendingTileKeys.insert(request.key);
        // Tile workers can outlive this non-QObject scheduler while the Qt context survives.
        const std::weak_ptr<DecodeLifetime> lifetime = m_decodeLifetime;
        const TileKey key = request.key;
        std::shared_ptr<ImageTileSource> source = plan.source;
        runAsyncWorker(
            m_context,
            [source, request]() mutable {
                QString errorString;
                std::optional<DecodedTile> tile = source->decodeTile(request, &errorString);
                return tile;
            },
            [this, lifetime, generation, key](std::optional<DecodedTile> tile) mutable {
                if (lifetime.expired()) {
                    return;
                }

                finishTileDecode(generation, key, std::move(tile));
            });
    }
}

bool ImageTileDecodeScheduler::tileRequestIsCurrent(quint64 generation, const TileKey &key) const
{
    return m_generation.accepts(generation) && m_pendingTileKeys.contains(key);
}

void ImageTileDecodeScheduler::finishTileDecode(
    quint64 generation, TileKey key, std::optional<DecodedTile> tile)
{
    if (!tileRequestIsCurrent(generation, key)) {
        return;
    }

    m_pendingTileKeys.remove(key);
    if (!tile.has_value()) {
        m_failedTileKeys.insert(key);
        return;
    }

    if (m_tileDecoded) {
        m_tileDecoded(std::move(*tile));
    }
}
}
