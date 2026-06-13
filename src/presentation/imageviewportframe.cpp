// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageviewportframe.h"

#include "bridge/qtgeometryconversion.h"
#include "kiriview/src/policy/imageviewportgeometry.cxx.h"

namespace kiriview {
ImageViewportFrame projectImageViewportFrame(
    const QSizeF &viewportSize, const QSizeF &displaySize, const QPointF &contentPosition)
{
    const RustImageViewportFrame frame = rustImageViewportFrame(
        Bridge::rustSizeF<RustSizeF>(viewportSize), Bridge::rustSizeF<RustSizeF>(displaySize),
        Bridge::rustPointF<RustPointF>(contentPosition));
    return ImageViewportFrame {
        Bridge::qtRectF(frame.image_rect),
        Bridge::qtSizeF(frame.content_size),
        Bridge::qtPointF(frame.maximum_content_position),
        Bridge::qtPointF(frame.content_position),
        Bridge::qtRectF(frame.visible_item_rect),
        frame.horizontal_pannable,
        frame.vertical_pannable,
        frame.pannable,
    };
}
}
