// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTTYPES_H
#define KIRIVIEW_IMAGEDOCUMENTTYPES_H

#include <QtGlobal>

namespace KiriView {
enum class ImageDocumentStatus {
    Null,
    Loading,
    Ready,
    Error,
};

enum class ImageDocumentChange {
    SourceUrl,
    Status,
    Loading,
    ErrorString,
    WindowTitleFileName,
    DisplayedUrl,
    ImageSize,
    ViewportSize,
    VisibleItemRect,
    DisplaySize,
    ZoomPercent,
    ZoomMode,
    PageNavigation,
    ContainerNavigation,
    FileDeletionInProgress,
    Repaint,
};

struct ImageDocumentRenderContext {
    qreal devicePixelRatio = 1.0;
    int maximumTextureSize = 0;
};
}

#endif
