// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTSOURCELOADRUNTIMEPLAN_H
#define KIRIVIEW_IMAGEDOCUMENTSOURCELOADRUNTIMEPLAN_H

#include "imagedocumenteffectplan.h"
#include "imagedocumentsourceloadpolicy.h"
#include "imagedocumentsourceloadrequest.h"

namespace KiriView {
ImageDocumentRuntimePlan imageDocumentSourceLoadRuntimePlan(
    const ImageDocumentSourceLoadRequest &request, const ImageDocumentSourceLoadPlan &plan);
}

#endif
