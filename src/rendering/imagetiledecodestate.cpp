// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetiledecodestate.h"

#include <utility>

namespace KiriView {
bool ImageTileDecodeExclusions::contains(const TileKey &key) const
{
    return pendingTileKeys.contains(key) || failedTileKeys.contains(key);
}

void ImageTileDecodeState::invalidate()
{
    m_generation.invalidate();
    clearRequests();
}

ImageTileDecodeExclusions ImageTileDecodeState::exclusions() const
{
    return ImageTileDecodeExclusions { m_pendingTileKeys, m_failedTileKeys };
}

ImageTileDecodeScheduleState ImageTileDecodeState::beginSchedule(
    const std::shared_ptr<DisplayedImageSurface> &surface)
{
    syncSurface(surface);
    return ImageTileDecodeScheduleState { m_generation.current(), exclusions() };
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
        if (schedule.exclusions.contains(request.key) || m_pendingTileKeys.contains(request.key)
            || m_failedTileKeys.contains(request.key)) {
            continue;
        }

        m_pendingTileKeys.insert(request.key);
        acceptedRequests.push_back(std::move(request));
    }
    return acceptedRequests;
}

bool ImageTileDecodeState::finish(quint64 generation, const TileKey &key)
{
    if (!m_generation.accepts(generation) || !m_pendingTileKeys.contains(key)) {
        return false;
    }

    m_pendingTileKeys.remove(key);
    return true;
}

void ImageTileDecodeState::fail(const TileKey &key) { m_failedTileKeys.insert(key); }

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
