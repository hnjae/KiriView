// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetiledecoderuntime.h"

#include "imagetiledecodeplan.h"

#include <utility>

namespace KiriView {
void ImageTileDecodeRuntime::invalidate() { m_decodeState.invalidate(); }

ImageTileDecodeRuntimePlan ImageTileDecodeRuntime::schedule(
    const std::shared_ptr<DisplayedImageSurface> &displayedSurface, const QSizeF &displaySize,
    const QRectF &visibleItemRect, const ImageDocumentRenderContext &context, int rotationDegrees)
{
    const ImageTileDecodeScheduleState schedule = m_decodeState.beginSchedule(displayedSurface);
    ImageTileDecodePlan candidatePlan = imageTileDecodePlan(displayedSurface, displaySize,
        visibleItemRect, context, rotationDegrees, schedule.exclusions);
    if (candidatePlan.isEmpty()) {
        return {};
    }

    std::vector<TileRequest> requests
        = m_decodeState.commitScheduleRequests(schedule, std::move(candidatePlan.requests));
    if (requests.empty()) {
        return {};
    }

    return ImageTileDecodeRuntimePlan {
        schedule.generation,
        std::move(candidatePlan.source),
        std::move(requests),
    };
}

bool ImageTileDecodeRuntime::acceptFinishedTileDecode(
    quint64 generation, const TileKey &key, bool decoded)
{
    if (!m_decodeState.finish(generation, key)) {
        return false;
    }

    if (!decoded) {
        m_decodeState.fail(key);
    }
    return true;
}
}
