// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetiledecodestate.h"

#include <utility>

namespace KiriView {
bool ImageTileDecodeExclusions::contains(const TileKey &key) const
{
    return pendingTileKeys.contains(key) || failedTileKeys.contains(key);
}

uint qHash(const ImageTileDecodeWorkKey &key, uint seed)
{
    return qHashMulti(seed, key.surfaceKey, key.tileKey);
}

void ImageTileDecodeState::invalidate()
{
    m_generation.invalidate();
    clearRequests();
}

ImageTileDecodeExclusions ImageTileDecodeState::exclusions(const RenderSurfaceKey &surfaceKey) const
{
    ImageTileDecodeExclusions result;
    for (const ImageTileDecodeWorkKey &key : m_pendingTileKeys) {
        if (sameRenderSurfaceKey(key.surfaceKey, surfaceKey)) {
            result.pendingTileKeys.insert(key.tileKey);
        }
    }
    for (const ImageTileDecodeWorkKey &key : m_failedTileKeys) {
        if (sameRenderSurfaceKey(key.surfaceKey, surfaceKey)) {
            result.failedTileKeys.insert(key.tileKey);
        }
    }
    return result;
}

ImageTileDecodeScheduleState ImageTileDecodeState::beginSchedule(
    const std::shared_ptr<DisplayedImageSurface> &surface, const RenderSurfaceKey &surfaceKey)
{
    syncSurface(surface);
    RenderSurfaceKey currentSurfaceKey = surfaceKey;
    currentSurfaceKey.surfaceGeneration = m_generation.current();
    return ImageTileDecodeScheduleState {
        m_generation.current(),
        currentSurfaceKey,
        exclusions(currentSurfaceKey),
    };
}

std::vector<TileRequest> ImageTileDecodeState::commitScheduleRequests(
    const ImageTileDecodeScheduleState &schedule, std::vector<TileRequest> requests)
{
    if (!m_generation.accepts(schedule.generation)) {
        return {};
    }

    std::vector<TileRequest> acceptedRequests;
    acceptedRequests.reserve(requests.size());
    for (TileRequest &request : requests) {
        const ImageTileDecodeWorkKey workKey { schedule.surfaceKey, request.key };
        if (schedule.exclusions.contains(request.key) || m_pendingTileKeys.contains(workKey)
            || m_failedTileKeys.contains(workKey)) {
            continue;
        }

        m_pendingTileKeys.insert(workKey);
        acceptedRequests.push_back(std::move(request));
    }
    return acceptedRequests;
}

bool ImageTileDecodeState::finish(
    quint64 generation, const RenderSurfaceKey &surfaceKey, const TileKey &key)
{
    const ImageTileDecodeWorkKey workKey { surfaceKey, key };
    if (!m_generation.accepts(generation) || !m_pendingTileKeys.contains(workKey)) {
        return false;
    }

    m_pendingTileKeys.remove(workKey);
    return true;
}

void ImageTileDecodeState::fail(const RenderSurfaceKey &surfaceKey, const TileKey &key)
{
    m_failedTileKeys.insert(ImageTileDecodeWorkKey { surfaceKey, key });
}

void ImageTileDecodeState::clearRequests()
{
    m_pendingTileKeys.clear();
    m_failedTileKeys.clear();
}

void ImageTileDecodeState::syncSurface(const std::shared_ptr<DisplayedImageSurface> &surface)
{
    if (m_surface.lock() == surface) {
        return;
    }

    m_surface = surface;
    m_generation.invalidate();
    clearRequests();
}
}
