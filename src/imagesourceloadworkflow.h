// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESOURCELOADWORKFLOW_H
#define KIRIVIEW_IMAGESOURCELOADWORKFLOW_H

#include "kiriview/src/imagesourceloadworkflow.cxx.h"

namespace KiriView {
namespace ImageSourceLoadWorkflow {
    ImageSourceLoadPlan plan(const ImageSourceLoadRequest &request);
}
}

#endif
