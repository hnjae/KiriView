// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagezoomworkflowstate.h"

#include <utility>

namespace KiriView {
ImageZoomWorkflowState::ImageZoomWorkflowState(RenderContextProvider renderContextProvider)
    : m_renderContextState(std::move(renderContextProvider))
{
}

const ImageZoomState &ImageZoomWorkflowState::zoomState() const { return m_zoomState; }

ImageDocumentRenderContext ImageZoomWorkflowState::renderContext() const
{
    return m_renderContextState.current();
}

qreal ImageZoomWorkflowState::devicePixelRatio() const
{
    return m_renderContextState.devicePixelRatio();
}

void ImageZoomWorkflowState::clear() { m_zoomState = ImageZoomState {}; }

void ImageZoomWorkflowState::clearContainer() { m_zoomState.clearContainer(); }

ImageZoomWorkflowMutationResult ImageZoomWorkflowState::mutate(
    const ZoomStateMutation &mutation, bool forceDisplayProjectionUpdate)
{
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    const ImageRenderContextChange renderContextChange = m_renderContextState.refresh();

    mutation(m_zoomState, renderContextChange.current.devicePixelRatio);

    return ImageZoomWorkflowMutationResult {
        ImageZoomState::changeSet(previous, renderContextChange.previous.devicePixelRatio,
            m_zoomState.snapshot(), renderContextChange.current.devicePixelRatio,
            forceDisplayProjectionUpdate),
        renderContextChange,
    };
}
}
