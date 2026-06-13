// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagerendercontextstate.h"

#include "rendering/imagerendering.h"

#include <utility>

namespace kiriview {
ImageRenderContextState::ImageRenderContextState(Provider provider)
    : m_provider(std::move(provider))
    , m_context(providedContext())
{
}

ImageDocumentRenderContext ImageRenderContextState::current() const { return m_context; }

qreal ImageRenderContextState::devicePixelRatio() const { return m_context.devicePixelRatio; }

ImageRenderContextChange ImageRenderContextState::refresh()
{
    const ImageDocumentRenderContext previous = m_context;
    m_context = providedContext();
    return ImageRenderContextChange { previous, m_context };
}

ImageDocumentRenderContext ImageRenderContextState::providedContext() const
{
    const ImageDocumentRenderContext context
        = m_provider ? m_provider() : ImageDocumentRenderContext {};
    return normalizedImageDocumentRenderContext(context);
}
}
