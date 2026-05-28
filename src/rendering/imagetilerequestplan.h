// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGETILEREQUESTPLAN_H
#define KIRIVIEW_IMAGETILEREQUESTPLAN_H

#include "imagesurfacedrawcontext.h"
#include "imagetiledecodestate.h"

#include <memory>
#include <vector>

namespace KiriView {
class ImageTileSource;
class StaticTileSurface;

struct ImageTileRequestPlanInput {
    const StaticTileSurface *surface = nullptr;
    ImageSurfaceDrawContext drawContext;
    ImageTileDecodeExclusions tileDecodeExclusions;
    bool resolutionIndependent = false;
};

struct ImageTileRequestPlan {
    ActiveTileLayer activeTileLayer;
    std::shared_ptr<ImageTileSource> source;
    std::vector<TileRequest> missingRequests;

    bool hasRequests() const { return source != nullptr && !missingRequests.empty(); }
};

ImageTileRequestPlan planImageTileRequests(const ImageTileRequestPlanInput &input);
}

#endif
