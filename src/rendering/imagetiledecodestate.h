// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGETILEDECODESTATE_H
#define KIRIVIEW_IMAGETILEDECODESTATE_H

#include "async/imageasyncticket.h"
#include "imagesurface.h"
#include "imagetile.h"

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
    ImageTileDecodeExclusions exclusions;
};

class ImageTileDecodeState final
{
public:
    void invalidate();
    ImageTileDecodeScheduleState beginSchedule(
        const std::shared_ptr<DisplayedImageSurface> &surface);
    std::vector<TileRequest> startRequests(quint64 generation, std::vector<TileRequest> requests);
    bool finish(quint64 generation, const TileKey &key);
    void fail(const TileKey &key);

private:
    ImageTileDecodeExclusions exclusions() const;
    void clearRequests();
    void syncSurface(const std::shared_ptr<DisplayedImageSurface> &surface);

    std::weak_ptr<DisplayedImageSurface> m_surface;
    ImageAsyncTicket m_generation;
    QSet<TileKey> m_pendingTileKeys;
    QSet<TileKey> m_failedTileKeys;
};
}

#endif
