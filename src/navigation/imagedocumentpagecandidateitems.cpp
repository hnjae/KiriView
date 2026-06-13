// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpagecandidateitems.h"

#include "archive/archiveformat.h"
#include "decoding/imageformatregistry.h"
#include "imagedocumentpagenavigationpolicy.h"
#include "location/imageurl.h"
#include "mediaformatregistry.h"
#include "navigationlogging.h"

#include <QDebug>
#include <cstddef>

namespace kiriview {
std::vector<ImageDocumentPageCandidate> imageDocumentPageNavigationCandidates(
    const KFileItemList &items)
{
    std::vector<ImageDocumentPageCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(items.size()));

    for (const KFileItem &item : items) {
        const QString name = item.name();
        if (!item.isFile() || !kiriview::isSupportedOrdinaryMediaFileName(name)) {
            continue;
        }

        candidates.push_back(ImageDocumentPageCandidate { item.url(), name,
            kiriview::isSupportedDirectVideoFileName(name) ? ImageDocumentPageKind::Video
                                                           : ImageDocumentPageKind::Image });
    }

    sortImageDocumentPageCandidates(&candidates);
    return candidates;
}

std::vector<DirectMediaNavigationCandidate> directMediaNavigationCandidates(
    const KFileItemList &items)
{
    const std::vector<ImageDocumentPageCandidate> imageDocumentPageCandidates
        = imageDocumentPageNavigationCandidates(items);
    std::vector<DirectMediaNavigationCandidate> candidates;
    candidates.reserve(imageDocumentPageCandidates.size());
    for (const ImageDocumentPageCandidate &candidate : imageDocumentPageCandidates) {
        candidates.push_back(DirectMediaNavigationCandidate { candidate.url, candidate.name });
    }

    qCDebug(kiriviewNavigationLog)
        << "direct media navigation candidates projected"
        << "items" << items.size() << "supportedCandidates" << candidates.size();
    for (std::size_t index = 0; index < candidates.size() && index < 8; ++index) {
        qCDebug(kiriviewNavigationLog) << "direct media navigation candidate"
                                       << "index" << index << "name" << candidates.at(index).name
                                       << "url" << candidates.at(index).url;
    }
    if (candidates.size() > 8) {
        qCDebug(kiriviewNavigationLog) << "direct media navigation candidates omitted"
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
            && kiriview::isComicBookArchiveFileName(name)) {
            candidates.push_back(
                ContainerNavigationCandidate { normalizedFileContainerUrl(item.url()), name,
                    ContainerNavigationCandidateType::ComicBookArchive });
        }
    }

    sortContainerNavigationCandidates(&candidates);
    return candidates;
}
}
