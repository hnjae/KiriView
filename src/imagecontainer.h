// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECONTAINER_H
#define KIRIVIEW_IMAGECONTAINER_H

#include "imagenavigationtypes.h"

#include <KFileItem>
#include <KIO/UDSEntry>
#include <QString>
#include <QUrl>
#include <optional>
#include <vector>

namespace KiriView {
std::optional<QUrl> comicBookArchiveRootUrl(const QUrl &url);
bool isUrlInsideArchiveRoot(const QUrl &url, const QUrl &archiveRootUrl);
std::optional<QUrl> containingComicBookArchiveRootUrl(const QUrl &url);
QString windowTitleFileNameForDisplayedUrl(
    const QUrl &displayedUrl, const QUrl &displayedComicBookRootUrl);
std::vector<ContainerNavigationCandidate> containerNavigationCandidates(const KFileItemList &items);
void appendArchiveImageNavigationCandidates(std::vector<ImageNavigationCandidate> *candidates,
    const KIO::UDSEntryList &entries, const QUrl &directoryUrl, const QUrl &archiveRootUrl);
QUrl containerNavigationUrlForImage(const QUrl &imageUrl, const QUrl &comicBookRootUrl);
}

#endif
