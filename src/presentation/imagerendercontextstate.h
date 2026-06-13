// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGERENDERCONTEXTSTATE_H
#define KIRIVIEW_IMAGERENDERCONTEXTSTATE_H

#include "rendering/imagerendercontext.h"

#include <QtGlobal>
#include <functional>

namespace kiriview {
struct ImageRenderContextChange {
    ImageDocumentRenderContext previous;
    ImageDocumentRenderContext current;
};

class ImageRenderContextState final
{
public:
    using Provider = std::function<ImageDocumentRenderContext()>;

    explicit ImageRenderContextState(Provider provider = {});

    ImageDocumentRenderContext current() const;
    qreal devicePixelRatio() const;
    ImageRenderContextChange refresh();

private:
    ImageDocumentRenderContext providedContext() const;

    Provider m_provider;
    ImageDocumentRenderContext m_context;
};
}

#endif
