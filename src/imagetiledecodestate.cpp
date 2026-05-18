// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetiledecodestate.h"

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

quint64 ImageTileDecodeState::beginSchedule(const std::shared_ptr<DisplayedImageSurface> &surface)
{
    syncSurface(surface);
    return m_generation.current();
}

ImageTileDecodeExclusions ImageTileDecodeState::exclusions() const
{
    return ImageTileDecodeExclusions { m_pendingTileKeys, m_failedTileKeys };
}

void ImageTileDecodeState::start(const TileKey &key) { m_pendingTileKeys.insert(key); }

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
