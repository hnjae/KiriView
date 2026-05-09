// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECONTAINER_H
#define KIRIVIEW_IMAGECONTAINER_H

#include "imagelocation.h"
#include "imagenavigationtypes.h"

#include <KFileItem>
#include <QString>
#include <QUrl>
#include <optional>
#include <vector>

namespace KiriView {
std::optional<QUrl> comicBookArchiveRootUrl(const QUrl &url);
std::optional<QUrl> directArchiveOpenRootUrl(const QUrl &url);
std::optional<ArchiveDocumentLocation> archiveDocumentLocationForLocalArchiveUrl(const QUrl &url);
bool isUrlInsideArchiveRoot(const QUrl &url, const QUrl &archiveRootUrl);
bool archiveDocumentContainsUrl(const ArchiveDocumentLocation &archiveDocument, const QUrl &url);
bool displayedLocationIsInsideArchiveDocument(const DisplayedImageLocation &location);
std::optional<QUrl> containingComicBookArchiveRootUrl(const QUrl &url);
std::optional<QUrl> containingDirectArchiveOpenRootUrl(const QUrl &url);
QString windowTitleFileNameForDisplayedLocation(const DisplayedImageLocation &location);
std::vector<ContainerNavigationCandidate> containerNavigationCandidates(const KFileItemList &items);
QUrl zoomScopeUrlForLocation(const DisplayedImageLocation &location);
QUrl containerNavigationUrlForLocation(const DisplayedImageLocation &location);
bool shouldResetRightToLeftReadingForLoad(bool rightToLeftReadingEnabled,
    const ArchiveDocumentLocation &displayedArchiveDocument, const QUrl &sourceUrl,
    const QUrl &containerNavigationUrl);
}

#endif
