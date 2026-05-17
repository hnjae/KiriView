// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOADPLAN_H
#define KIRIVIEW_IMAGELOADPLAN_H

#include "imageloadtypes.h"

#include <QtGlobal>

namespace KiriView {
struct ImageArchiveLoadPlan {
    ArchiveDocumentLocation archiveDocument;
    bool requiresArchiveListing = false;
};

struct ImageLoadPlan {
    ImageLoadSession session;
    bool requiresArchiveListing = false;
};

ImageArchiveLoadPlan imageArchiveLoadPlan(const ImageLoadRequest &request);
ImageLoadPlan imageLoadPlan(quint64 id, ImageLoadRequest request);
}

#endif
