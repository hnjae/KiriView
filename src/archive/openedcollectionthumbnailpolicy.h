// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_OPENEDCOLLECTIONTHUMBNAILPOLICY_H
#define KIRIVIEW_OPENEDCOLLECTIONTHUMBNAILPOLICY_H

#include "location/imagelocation.h"
#include "navigation/imagedocumentpagenavigationtypes.h"

#include <QUrl>

namespace KiriView {
enum class OpenedCollectionThumbnailSourcePlanKind {
    Unsupported,
    CacheableOpenedCollectionEntry,
};

struct OpenedCollectionThumbnailSourcePlan {
    OpenedCollectionThumbnailSourcePlanKind kind
        = OpenedCollectionThumbnailSourcePlanKind::Unsupported;
    OpenedCollectionScopeLocation openedCollectionScope;
};

OpenedCollectionThumbnailSourcePlan openedCollectionThumbnailSourcePlan(
    const OpenedCollectionScopeLocation &openedCollectionScope, const QUrl &entryUrl,
    ImageDocumentPageKind pageKind);
}

#endif
