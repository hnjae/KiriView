// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESOURCELOADWORKFLOW_H
#define KIRIVIEW_IMAGESOURCELOADWORKFLOW_H

#include "kiriview/src/imagesourceloadworkflow.cxx.h"

namespace KiriView {
namespace ImageSourceLoadWorkflow {
    ImageSourceLoadRightToLeftReadingChange rightToLeftReadingChangeForLoad(
        bool resetRightToLeftReading, bool rightToLeftReadingEnabled);
    ImageSourceLoadPlan plan(const ImageSourceLoadRequest &request);
}
}

#endif
