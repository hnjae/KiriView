// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOADPLAN_H
#define KIRIVIEW_IMAGELOADPLAN_H

#include "imageloadtypes.h"

#include <QtGlobal>

namespace KiriView {
struct ImageLoadPlan {
    ImageLoadSession session;
    bool requiresArchiveListing = false;
};

ImageLoadPlan imageLoadPlan(quint64 id, ImageLoadRequest request);
}

#endif
