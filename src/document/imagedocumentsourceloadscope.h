// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTSOURCELOADSCOPE_H
#define KIRIVIEW_IMAGEDOCUMENTSOURCELOADSCOPE_H

#include "imagedocumentsourceloadrequest.h"
#include "location/imagelocation.h"

namespace kiriview {
OpenedCollectionScopeLocation openedCollectionScopeForImageDocumentSourceLoad(
    const ImageDocumentSourceLoadRequest &request,
    const OpenedCollectionScopeLocation &displayedOpenedCollectionScope);
}

#endif
