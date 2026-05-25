// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/imagezoomstateconversion.h"

#include "bridge/qtgeometryconversion.h"

#include <QUrl>

namespace KiriView::Bridge {
RustImageZoomMode rustImageZoomMode(ImageZoomMode zoomMode)
{
    switch (zoomMode) {
    case ImageZoomMode::Fit:
        return RustImageZoomMode::Fit;
    case ImageZoomMode::FitHeight:
        return RustImageZoomMode::FitHeight;
    case ImageZoomMode::FitWidth:
        return RustImageZoomMode::FitWidth;
    case ImageZoomMode::Manual:
        return RustImageZoomMode::Manual;
    }

    return RustImageZoomMode::Fit;
}

ImageZoomMode imageZoomModeFromRust(RustImageZoomMode zoomMode)
{
    switch (zoomMode) {
    case RustImageZoomMode::Fit:
        return ImageZoomMode::Fit;
    case RustImageZoomMode::FitHeight:
        return ImageZoomMode::FitHeight;
    case RustImageZoomMode::FitWidth:
        return ImageZoomMode::FitWidth;
    case RustImageZoomMode::Manual:
        return ImageZoomMode::Manual;
    }

    return ImageZoomMode::Fit;
}

RustImageZoomState rustImageZoomState(const ImageZoomSnapshot &snapshot)
{
    return RustImageZoomState { snapshot.imageSize.width(), snapshot.imageSize.height(),
        snapshot.viewportSize.width(), snapshot.viewportSize.height(), snapshot.displaySize.width(),
        snapshot.displaySize.height(), snapshot.zoomPercent, rustImageZoomMode(snapshot.zoomMode) };
}

ImageZoomSnapshot imageZoomSnapshotFromRust(
    const RustImageZoomState &state, const QUrl &containerUrl)
{
    return ImageZoomSnapshot {
        qtSize(RustZoomSize { state.image_width, state.image_height }),
        qtSizeF(state.viewport_width, state.viewport_height),
        qtSizeF(state.display_width, state.display_height),
        state.zoom_percent,
        imageZoomModeFromRust(state.zoom_mode),
        containerUrl,
    };
}

RustLoadedImageZoom rustLoadedImageZoom(const LoadedImageZoom &zoom)
{
    return RustLoadedImageZoom { rustImageZoomMode(zoom.zoomMode), zoom.zoomPercent,
        zoom.displaySize.width(), zoom.displaySize.height() };
}

LoadedImageZoom loadedImageZoomFromRust(const RustLoadedImageZoom &zoom)
{
    return LoadedImageZoom {
        imageZoomModeFromRust(zoom.zoom_mode),
        zoom.zoom_percent,
        qtSizeF(zoom.display_width, zoom.display_height),
    };
}
}
