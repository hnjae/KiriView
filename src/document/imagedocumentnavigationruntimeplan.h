// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTNAVIGATIONRUNTIMEPLAN_H
#define KIRIVIEW_IMAGEDOCUMENTNAVIGATIONRUNTIMEPLAN_H

#include "imagedocumentruntimeplan.h"
#include "navigation/imagedocumentpagenavigationplan.h"

namespace kiriview {
ImageDocumentRuntimePlan imageDocumentRuntimePlanForNavigationPlan(
    const ImageDocumentPageNavigationPlan& navigationPlan);
}

#endif
