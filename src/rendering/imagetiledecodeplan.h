// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGETILEDECODEPLAN_H
#define KIRIVIEW_IMAGETILEDECODEPLAN_H

#include "document/imagedocumenttypes.h"
#include "imagesurface.h"
#include "imagetile.h"
#include "imagetiledecodestate.h"

#include <QRectF>
#include <QSizeF>
#include <QtGlobal>
#include <memory>
#include <vector>

namespace KiriView {
class ImageTileSource;

struct ImageTileDecodePlan {
    std::shared_ptr<ImageTileSource> source;
    std::vector<TileRequest> requests;

    bool isEmpty() const { return source == nullptr || requests.empty(); }
};

struct ImageTileDecodeSchedulePlan {
    quint64 generation = 0;
    std::shared_ptr<ImageTileSource> source;
    std::vector<TileRequest> requests;

    bool isEmpty() const { return source == nullptr || requests.empty(); }
};

ImageTileDecodePlan imageTileDecodePlan(
    const std::shared_ptr<DisplayedImageSurface> &displayedSurface, const QSizeF &displaySize,
    const QRectF &visibleItemRect, const ImageDocumentRenderContext &context, int rotationDegrees,
    const ImageTileDecodeExclusions &exclusions);

ImageTileDecodeSchedulePlan imageTileDecodeSchedulePlan(ImageTileDecodeState &state,
    const std::shared_ptr<DisplayedImageSurface> &displayedSurface, const QSizeF &displaySize,
    const QRectF &visibleItemRect, const ImageDocumentRenderContext &context, int rotationDegrees);
}

#endif
