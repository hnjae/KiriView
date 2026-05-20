// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECONTAINER_H
#define KIRIVIEW_IMAGECONTAINER_H

#include "imagenavigationtypes.h"
#include "location/imagelocation.h"

#include <QString>
#include <QUrl>
#include <optional>

namespace KiriView {
std::optional<QUrl> comicBookArchiveRootUrl(const QUrl &url);
std::optional<QUrl> directArchiveOpenRootUrl(const QUrl &url);
std::optional<ArchiveDocumentLocation> archiveDocumentLocationForLocalArchiveUrl(const QUrl &url);
std::optional<ArchiveDocumentLocation> directOpenDocumentLocationForLocalUrl(const QUrl &url);
bool isUrlInsideArchiveRoot(const QUrl &url, const QUrl &archiveRootUrl);
bool archiveDocumentContainsUrl(const ArchiveDocumentLocation &archiveDocument, const QUrl &url);
bool displayedLocationIsInsideArchiveDocument(const DisplayedImageLocation &location);
std::optional<QUrl> containingComicBookArchiveRootUrl(const QUrl &url);
std::optional<QUrl> containingDirectArchiveOpenRootUrl(const QUrl &url);
QString windowTitleFileNameForDisplayedLocation(const DisplayedImageLocation &location);
QUrl zoomScopeUrlForLocation(const DisplayedImageLocation &location);
QUrl containerNavigationUrlForLocation(const DisplayedImageLocation &location);
}

#endif
