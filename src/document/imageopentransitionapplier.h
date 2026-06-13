// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENTRANSITIONAPPLIER_H
#define KIRIVIEW_IMAGEOPENTRANSITIONAPPLIER_H

#include "imageopenapplicationplan.h"
#include "imageopentransition.h"

namespace kiriview {
class ImageDocumentState;

ImageOpenApplicationPlan imageOpenApplicationPlan(
    const ImageOpenTransition &transition, const ImageOpenTransitionContext &context = {});
ImageDocumentRuntimePlan applyImageOpenApplicationPlan(
    ImageDocumentState &state, ImageOpenApplicationPlan plan);
ImageDocumentRuntimePlan applyImageOpenTransition(ImageDocumentState &state,
    const ImageOpenTransition &transition, const ImageOpenTransitionContext &context = {});
}

#endif
