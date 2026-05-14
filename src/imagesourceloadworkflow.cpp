// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagesourceloadworkflow.h"

namespace KiriView::ImageSourceLoadWorkflow {
ImageSourceLoadRightToLeftReadingChange rightToLeftReadingChangeForLoad(
    bool resetRightToLeftReading, bool rightToLeftReadingEnabled)
{
    return rustImageSourceLoadRightToLeftReadingChangeForLoad(
        resetRightToLeftReading, rightToLeftReadingEnabled);
}

ImageSourceLoadPlan plan(const ImageSourceLoadRequest &request)
{
    return rustImageSourceLoadPlan(request);
}
}
