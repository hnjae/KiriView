// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "openedcollectionthumbnailpolicy.h"

#include "location/imagedocumentlocation.h"

namespace KiriView {
bool openedCollectionRootSchemeSupportsThumbnailContentIdentity(const QString &rootScheme)
{
    return rootScheme == QStringLiteral("zip") || rootScheme == QStringLiteral("sevenz");
}

OpenedCollectionThumbnailSourcePlan openedCollectionThumbnailSourcePlan(
    const OpenedCollectionScopeLocation &openedCollectionScope, const QUrl &entryUrl,
    ImageDocumentPageKind pageKind)
{
    if (pageKind != ImageDocumentPageKind::Image || !openedCollectionScope.isComicBook()
        || !openedCollectionRootSchemeSupportsThumbnailContentIdentity(
            openedCollectionScope.rootUrl().scheme())
        || !openedCollectionScopeContainsUrl(openedCollectionScope, entryUrl)) {
        return {};
    }

    return OpenedCollectionThumbnailSourcePlan {
        OpenedCollectionThumbnailSourcePlanKind::CacheableOpenedCollectionEntry,
        openedCollectionScope,
    };
}
}
