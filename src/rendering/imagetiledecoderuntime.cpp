// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetiledecoderuntime.h"

#include "imagerenderframe.h"

#include <QString>
#include <utility>

namespace {
QString pageRoleKey(KiriView::DisplayedPageRole pageRole)
{
    switch (pageRole) {
    case KiriView::DisplayedPageRole::Primary:
        return QStringLiteral("primary");
    case KiriView::DisplayedPageRole::Secondary:
        return QStringLiteral("secondary");
    }

    return QStringLiteral("unknown");
}

QString renderSourceFamily(const KiriView::DisplayedImageSurface *surface)
{
    if (surface == nullptr) {
        return {};
    }

    const KiriView::StaticTileSurface *staticSurface = surface->staticTileSurface();
    if (staticSurface == nullptr || staticSurface->source() == nullptr) {
        return QStringLiteral("legacy");
    }

    return staticSurface->source()->isResolutionIndependent()
        ? QStringLiteral("resolution-independent")
        : QStringLiteral("raster");
}

KiriView::RenderSurfaceKey renderSurfaceKeyForTileDecode(
    const KiriView::DisplayedImageSurface *surface, quint64 renderContextGeneration,
    KiriView::DisplayedPageRole pageRole)
{
    return KiriView::renderSurfaceKey(
        surface == nullptr ? QString() : QString::number(surface->identity()), 0,
        renderContextGeneration, pageRoleKey(pageRole), renderSourceFamily(surface));
}
}

namespace KiriView {
void ImageTileDecodeRuntime::invalidate() { m_decodeState.invalidate(); }

ImageTileDecodeRuntimePlan ImageTileDecodeRuntime::schedule(
    const std::shared_ptr<DisplayedImageSurface> &displayedSurface, const QSizeF &displaySize,
    const QRectF &visibleItemRect, const ImageDocumentRenderContext &context, int rotationDegrees)
{
    constexpr DisplayedPageRole pageRole = DisplayedPageRole::Primary;
    const ImageTileDecodeScheduleState schedule = m_decodeState.beginSchedule(displayedSurface,
        renderSurfaceKeyForTileDecode(displayedSurface.get(), context.generation, pageRole));
    ImageRenderFrame frame = projectImageRenderFrame(ImageRenderFrameInput {
        displayedSurface.get(),
        schedule.generation,
        context.generation,
        ImageSurfaceDrawContext {
            QRectF(0.0, 0.0, displaySize.width(), displaySize.height()),
            displaySize,
            visibleItemRect,
            context.devicePixelRatio,
            rotationDegrees,
        },
        pageRole,
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
        schedule.surfaceKey,
        std::move(frame.tileRequestSource),
        std::move(requests),
    };
}

bool ImageTileDecodeRuntime::acceptFinishedTileDecode(
    quint64 generation, const RenderSurfaceKey &surfaceKey, const TileKey &key, bool decoded)
{
    if (!m_decodeState.finish(generation, surfaceKey, key)) {
        return false;
    }

    if (!decoded) {
        m_decodeState.fail(surfaceKey, key);
    }
    return true;
}
}
