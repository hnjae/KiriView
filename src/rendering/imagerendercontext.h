// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGERENDERCONTEXT_H
#define KIRIVIEW_IMAGERENDERCONTEXT_H

#include <QtGlobal>

namespace kiriview {
enum class DisplayedPageRole {
    Primary,
    Secondary,
};

struct ImageDocumentRenderContext
{
    qreal devicePixelRatio = 1.0;
    int maximumTextureSize = 0;
    quint64 generation = 0;
};
}

#endif
