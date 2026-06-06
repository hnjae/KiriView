// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "openedcollectionthumbnailpolicy.h"

#include "decoding/imageformatregistry.h"
#include "location/imagedocumentlocation.h"

namespace {
bool rootSchemeSupportsThumbnailContentIdentity(const QString &rootScheme)
{
    return rootScheme == QStringLiteral("zip") || rootScheme == QStringLiteral("sevenz");
}
}

namespace KiriView {
bool openedCollectionEntrySupportsThumbnailContentIdentity(
    const OpenedCollectionScopeLocation &openedCollectionScope, ImageDocumentPageKind pageKind)
{
    return pageKind == ImageDocumentPageKind::Image && openedCollectionScope.isComicBook()
        && rootSchemeSupportsThumbnailContentIdentity(openedCollectionScope.rootUrl().scheme());
}

bool openedCollectionEntryPathSupportsThumbnailContentIdentity(
    const OpenedCollectionScopeLocation &openedCollectionScope, const QString &entryPath)
{
    return isSupportedImageFileName(entryPath)
        && openedCollectionEntrySupportsThumbnailContentIdentity(
            openedCollectionScope, ImageDocumentPageKind::Image);
}

OpenedCollectionThumbnailSourcePlan openedCollectionThumbnailSourcePlan(
    const OpenedCollectionScopeLocation &openedCollectionScope, const QUrl &entryUrl,
    ImageDocumentPageKind pageKind)
{
    if (!openedCollectionEntrySupportsThumbnailContentIdentity(openedCollectionScope, pageKind)
        || !openedCollectionScopeContainsUrl(openedCollectionScope, entryUrl)) {
        return {};
    }

    return OpenedCollectionThumbnailSourcePlan {
        OpenedCollectionThumbnailSourcePlanKind::CacheableOpenedCollectionEntry,
        openedCollectionScope,
    };
}
}
