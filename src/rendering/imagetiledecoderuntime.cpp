// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetiledecoderuntime.h"

#include "imagerenderframe.h"

#include <utility>

namespace KiriView {
void ImageTileDecodeRuntime::invalidate() { m_decodeState.invalidate(); }

ImageTileDecodeRuntimePlan ImageTileDecodeRuntime::schedule(
    const std::shared_ptr<DisplayedImageSurface> &displayedSurface, const QSizeF &displaySize,
    const QRectF &visibleItemRect, const ImageDocumentRenderContext &context, int rotationDegrees)
{
    const ImageTileDecodeScheduleState schedule = m_decodeState.beginSchedule(displayedSurface);
    ImageRenderFrame frame = projectImageRenderFrame(ImageRenderFrameInput {
        displayedSurface.get(),
        schedule.generation,
        ImageSurfaceDrawContext {
            QRectF(0.0, 0.0, displaySize.width(), displaySize.height()),
            displaySize,
            visibleItemRect,
            context.devicePixelRatio,
            rotationDegrees,
        },
        DisplayedPageRole::Primary,
        schedule.exclusions,
    });
    if (frame.tileRequestSource == nullptr || frame.missingTileRequests.empty()) {
        return {};
    }

    std::vector<TileRequest> requests
        = m_decodeState.commitScheduleRequests(schedule, std::move(frame.missingTileRequests));
    if (requests.empty()) {
        return {};
    }

    return ImageTileDecodeRuntimePlan {
        schedule.generation,
        std::move(frame.tileRequestSource),
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
