// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGETILEDECODESTATE_H
#define KIRIVIEW_IMAGETILEDECODESTATE_H

#include "async/imageasyncticket.h"
#include "imagesurface.h"
#include "imagetile.h"
#include "location/sourcekey.h"

#include <QSet>
#include <memory>
#include <vector>

namespace KiriView {
struct ImageTileDecodeExclusions {
    QSet<TileKey> pendingTileKeys;
    QSet<TileKey> failedTileKeys;

    bool contains(const TileKey &key) const;
};

struct ImageTileDecodeScheduleState {
    quint64 generation = 0;
    RenderSurfaceKey surfaceKey;
    ImageTileDecodeExclusions exclusions;
};

struct ImageTileDecodeWorkKey {
    RenderSurfaceKey surfaceKey;
    TileKey tileKey;

    friend bool operator==(const ImageTileDecodeWorkKey &left, const ImageTileDecodeWorkKey &right)
    {
        return sameRenderSurfaceKey(left.surfaceKey, right.surfaceKey)
            && left.tileKey == right.tileKey;
    }
};

uint qHash(const ImageTileDecodeWorkKey &key, uint seed = 0);

class ImageTileDecodeState final
{
public:
    void invalidate();
    ImageTileDecodeScheduleState beginSchedule(
        const std::shared_ptr<DisplayedImageSurface> &surface, const RenderSurfaceKey &surfaceKey);
    std::vector<TileRequest> commitScheduleRequests(
        const ImageTileDecodeScheduleState &schedule, std::vector<TileRequest> requests);
    bool finish(quint64 generation, const RenderSurfaceKey &surfaceKey, const TileKey &key);
    void fail(const RenderSurfaceKey &surfaceKey, const TileKey &key);

private:
    ImageTileDecodeExclusions exclusions(const RenderSurfaceKey &surfaceKey) const;
    void clearRequests();
    void syncSurface(const std::shared_ptr<DisplayedImageSurface> &surface);

    std::weak_ptr<DisplayedImageSurface> m_surface;
    ImageAsyncTicket m_generation;
    QSet<ImageTileDecodeWorkKey> m_pendingTileKeys;
    QSet<ImageTileDecodeWorkKey> m_failedTileKeys;
};
}

#endif
