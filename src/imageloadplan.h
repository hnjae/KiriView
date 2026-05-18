// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOADPLAN_H
#define KIRIVIEW_IMAGELOADPLAN_H

#include "imageloadtypes.h"

#include <QtGlobal>

namespace KiriView {
enum class ImageArchiveLoadEffect {
    ReadImage,
    LoadImageCandidates,
};

enum class ImageLoadStartEffect {
    DecodeImage,
    LoadArchiveImageCandidates,
};

struct ImageArchiveLoadPlan {
    ArchiveDocumentLocation archiveDocument;
    ImageArchiveLoadEffect effect = ImageArchiveLoadEffect::ReadImage;
};

struct ImageLoadPlan {
    ImageLoadSession session;
    ImageLoadStartEffect startEffect = ImageLoadStartEffect::DecodeImage;
};

ImageArchiveLoadPlan imageArchiveLoadPlan(const ImageLoadRequest &request);
ImageLoadPlan imageLoadPlan(quint64 id, ImageLoadRequest request);
}

#endif
