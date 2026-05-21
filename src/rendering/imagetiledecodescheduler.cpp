// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetiledecodescheduler.h"

#include "async/imageasyncworker.h"
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

void ImageTileDecodeScheduler::invalidate() { m_decodeState.invalidate(); }

void ImageTileDecodeScheduler::schedule(
    const std::shared_ptr<DisplayedImageSurface> &displayedSurface, const QSizeF &displaySize,
    const QRectF &visibleItemRect, const ImageDocumentRenderContext &context, int rotationDegrees)
{
    const ImageTileDecodeSchedulePlan plan = imageTileDecodeSchedulePlan(
        m_decodeState, displayedSurface, displaySize, visibleItemRect, context, rotationDegrees);
    if (plan.isEmpty()) {
        return;
    }

    for (const TileRequest &request : plan.requests) {
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
            [this, lifetime, generation = plan.generation, key](
                std::optional<DecodedTile> tile) mutable {
                if (lifetime.expired()) {
                    return;
                }

                finishTileDecode(generation, key, std::move(tile));
            });
    }
}

void ImageTileDecodeScheduler::finishTileDecode(
    quint64 generation, TileKey key, std::optional<DecodedTile> tile)
{
    if (!m_decodeState.finish(generation, key)) {
        return;
    }

    if (!tile.has_value()) {
        m_decodeState.fail(key);
        return;
    }

    if (m_tileDecoded) {
        m_tileDecoded(std::move(*tile));
    }
}
}
