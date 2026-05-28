// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidateitems.h"

#include "archive/archiveformat.h"
#include "decoding/imageformatregistry.h"
#include "imagenavigationmodel.h"
#include "location/imageurl.h"
#include "mediaformatregistry.h"
#include "navigationlogging.h"

#include <QDebug>
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

std::vector<MediaNavigationCandidate> mediaNavigationCandidates(const KFileItemList &items)
{
    const std::vector<ImageNavigationCandidate> imageCandidates = imageNavigationCandidates(items);
    std::vector<MediaNavigationCandidate> candidates;
    candidates.reserve(imageCandidates.size());
    for (const ImageNavigationCandidate &candidate : imageCandidates) {
        candidates.push_back(MediaNavigationCandidate { candidate.url, candidate.name });
    }

    qCDebug(kiriviewNavigationLog)
        << "media navigation candidates projected"
        << "items" << items.size() << "supportedCandidates" << candidates.size();
    for (std::size_t index = 0; index < candidates.size() && index < 8; ++index) {
        qCDebug(kiriviewNavigationLog) << "media navigation candidate"
                                       << "index" << index << "name" << candidates.at(index).name
                                       << "url" << candidates.at(index).url;
    }
    if (candidates.size() > 8) {
        qCDebug(kiriviewNavigationLog) << "media navigation candidates omitted"
                                       << "count" << candidates.size() - 8;
    }

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
