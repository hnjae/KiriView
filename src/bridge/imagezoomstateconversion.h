// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_BRIDGE_IMAGEZOOMSTATECONVERSION_H
#define KIRIVIEW_BRIDGE_IMAGEZOOMSTATECONVERSION_H

#include "kiriview/src/policy/imagezoomstate.cxx.h"
#include "presentation/imagezoomstate.h"

class QUrl;

namespace kiriview::Bridge {
RustImageZoomMode rustImageZoomMode(ImageZoomMode zoomMode);
ImageZoomMode imageZoomModeFromRust(RustImageZoomMode zoomMode);
RustImageZoomState rustImageZoomState(const ImageZoomSnapshot& snapshot);
ImageZoomSnapshot imageZoomSnapshotFromRust(
    const RustImageZoomState& state, const QUrl& containerUrl);
RustLoadedImageZoom rustLoadedImageZoom(const LoadedImageZoom& zoom);
LoadedImageZoom loadedImageZoomFromRust(const RustLoadedImageZoom& zoom);
}

#endif
