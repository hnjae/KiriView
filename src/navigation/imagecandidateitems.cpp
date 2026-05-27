// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidateitems.h"

#include "archive/archiveformat.h"
#include "decoding/imageformatregistry.h"
#include "imagenavigationmodel.h"
#include "location/imageurl.h"
#include "mediaformatregistry.h"

#include <cstddef>

namespace KiriView {
std::vector<ImageNavigationCandidate> imageNavigationCandidates(const KFileItemList &items)
{
    std::vector<ImageNavigationCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(items.size()));

    for (const KFileItem &item : items) {
        const QString name = item.name();
        if (!item.isFile() || !KiriView::isSupportedOrdinaryMediaFileName(name)) {
            continue;
        }

        candidates.push_back(ImageNavigationCandidate { item.url(), name,
            KiriView::isSupportedDirectVideoFileName(name) ? ImageNavigationCandidateKind::Video
                                                           : ImageNavigationCandidateKind::Image });
    }

    sortImageNavigationCandidates(&candidates);
    return candidates;
}

std::vector<ContainerNavigationCandidate> containerNavigationCandidates(const KFileItemList &items)
{
    std::vector<ContainerNavigationCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(items.size()));

    for (const KFileItem &item : items) {
        const QString name = item.name();
        if (item.isFile() && item.url().isLocalFile()
            && KiriView::isComicBookArchiveFileName(name)) {
            candidates.push_back(
                ContainerNavigationCandidate { normalizedFileContainerUrl(item.url()), name,
                    ContainerNavigationCandidateType::ComicBookArchive });
        }
    }

    sortContainerNavigationCandidates(&candidates);
    return candidates;
}
}
