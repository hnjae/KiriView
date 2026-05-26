// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGETILEDECODERUNTIME_H
#define KIRIVIEW_IMAGETILEDECODERUNTIME_H

#include "imagerendercontext.h"
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

struct ImageTileDecodeRuntimePlan {
    quint64 generation = 0;
    std::shared_ptr<ImageTileSource> source;
    std::vector<TileRequest> requests;

    bool isEmpty() const { return source == nullptr || requests.empty(); }
};

class ImageTileDecodeRuntime final
{
public:
    void invalidate();
    ImageTileDecodeRuntimePlan schedule(
        const std::shared_ptr<DisplayedImageSurface> &displayedSurface, const QSizeF &displaySize,
        const QRectF &visibleItemRect, const ImageDocumentRenderContext &context,
        int rotationDegrees = 0);
    bool acceptFinishedTileDecode(quint64 generation, const TileKey &key, bool decoded);

private:
    ImageTileDecodeState m_decodeState;
};
}

#endif
