// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentsourceloadscope.h"

#include "imageloadplan.h"

namespace kiriview {
OpenedCollectionScopeLocation openedCollectionScopeForImageDocumentSourceLoad(
    const ImageDocumentSourceLoadRequest &request,
    const OpenedCollectionScopeLocation &displayedOpenedCollectionScope)
{
    return openedCollectionScopeLoadPlan(
        ImageLoadRequest::fromTarget(
            ImageDocumentPageTarget { request.sourceUrl, request.sourceKind },
            displayedOpenedCollectionScope, request.containerNavigationUrl))
        .openedCollectionScope;
}
}
