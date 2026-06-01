// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "openedcollectionthumbnailpolicy.h"

#include "location/imagedocumentlocation.h"

namespace {
bool rootSchemeSupportsContentThumbnailIdentity(const QString &scheme)
{
    return scheme == QStringLiteral("zip") || scheme == QStringLiteral("sevenz");
}
}

namespace KiriView {
OpenedCollectionThumbnailSourcePlan openedCollectionThumbnailSourcePlan(
    const OpenedCollectionScopeLocation &openedCollectionScope, const QUrl &entryUrl,
    ImageDocumentPageKind pageKind)
{
    if (pageKind != ImageDocumentPageKind::Image || !openedCollectionScope.isComicBook()
        || !rootSchemeSupportsContentThumbnailIdentity(openedCollectionScope.rootUrl().scheme())
        || !openedCollectionScopeContainsUrl(openedCollectionScope, entryUrl)) {
        return {};
    }

    return OpenedCollectionThumbnailSourcePlan {
        OpenedCollectionThumbnailSourcePlanKind::CacheableOpenedCollectionEntry,
        openedCollectionScope,
    };
}
}
