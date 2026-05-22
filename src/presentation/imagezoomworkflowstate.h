// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEZOOMWORKFLOWSTATE_H
#define KIRIVIEW_IMAGEZOOMWORKFLOWSTATE_H

#include "presentation/imagerendercontextstate.h"
#include "presentation/imagezoomstate.h"

#include <functional>

namespace KiriView {
struct ImageZoomWorkflowMutationResult {
    ImageZoomChangeSet changes;
    ImageRenderContextChange renderContextChange;
};

class ImageZoomWorkflowState final
{
public:
    using RenderContextProvider = ImageRenderContextState::Provider;
    using ZoomStateMutation = std::function<void(ImageZoomState &, qreal devicePixelRatio)>;

    explicit ImageZoomWorkflowState(RenderContextProvider renderContextProvider = {});

    const ImageZoomState &zoomState() const;
    ImageDocumentRenderContext renderContext() const;
    qreal devicePixelRatio() const;

    void clear();
    void clearContainer();
    ImageZoomWorkflowMutationResult mutate(
        const ZoomStateMutation &mutation, bool forceTileRefresh = false);

private:
    ImageRenderContextState m_renderContextState;
    ImageZoomState m_zoomState;
};
}

#endif
