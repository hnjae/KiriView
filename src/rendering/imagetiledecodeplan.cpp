// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetiledecodeplan.h"

#include "imagerenderframe.h"

#include <utility>

namespace KiriView {
ImageTileDecodePlan imageTileDecodePlan(
    const std::shared_ptr<DisplayedImageSurface> &displayedSurface, const QSizeF &displaySize,
    const QRectF &visibleItemRect, const ImageDocumentRenderContext &context, int rotationDegrees,
    const ImageTileDecodeExclusions &exclusions)
{
    ImageRenderFrame frame = projectImageRenderFrame(ImageRenderFrameInput {
        displayedSurface.get(),
        0,
        ImageSurfaceDrawContext {
            QRectF(0.0, 0.0, displaySize.width(), displaySize.height()),
            displaySize,
            visibleItemRect,
            context.devicePixelRatio,
            rotationDegrees,
        },
        DisplayedPageRole::Primary,
        exclusions,
    });
    return ImageTileDecodePlan { frame.tileRequestSource, std::move(frame.missingTileRequests) };
}

}
