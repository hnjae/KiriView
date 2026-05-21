// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTEFFECTPLAN_H
#define KIRIVIEW_IMAGEDOCUMENTEFFECTPLAN_H

#include "imagedocumenteffects.h"
#include "imagedocumentruntimeplan.h"

namespace KiriView {
ImageDocumentRuntimePlan imageDocumentEffectPlan(const ImageDocumentEffect &effect);
ImageDocumentRuntimePlan imageDocumentShutdownPlan();
}

#endif
