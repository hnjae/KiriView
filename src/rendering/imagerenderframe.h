// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGERENDERFRAME_H
#define KIRIVIEW_IMAGERENDERFRAME_H

#include "document/imagedocumenttypes.h"
#include "imagerendering.h"
#include "imagetiledecodestate.h"

#include <memory>
#include <vector>

namespace KiriView {
class ImageTileSource;

struct ImageRenderFrameInput {
    const DisplayedImageSurface *surface = nullptr;
    quint64 surfaceRevision = 0;
    ImageSurfaceDrawContext drawContext;
    DisplayedPageRole pageRole = DisplayedPageRole::Primary;
    ImageTileDecodeExclusions tileDecodeExclusions;
};

struct ImageRenderFrame {
    quint64 surfaceIdentity = 0;
    quint64 surfaceRevision = 0;
    ImageSurfaceDrawContext drawContext;
    DisplayedPageRole pageRole = DisplayedPageRole::Primary;
    ActiveTileLayer activeTileLayer;
    std::vector<ImageSurfaceDrawEntry> drawEntries;
    std::vector<ImageSurfaceDrawIdentity> drawIdentities;
    std::shared_ptr<ImageTileSource> tileRequestSource;
    std::vector<TileRequest> missingTileRequests;
    bool renderable = false;

    bool isRenderable() const { return renderable; }
};

ImageRenderFrame projectImageRenderFrame(const ImageRenderFrameInput &input);
}

#endif
