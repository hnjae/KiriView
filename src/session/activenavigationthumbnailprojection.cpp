// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "activenavigationthumbnailprojection.h"

#include "navigation/mediaformatregistry.h"

#include <cstddef>

namespace {
kiriview::ActiveNavigationThumbnailKind thumbnailKindForDirectMediaNavigationCandidate(
    const kiriview::DirectMediaNavigationCandidate &candidate)
{
    return kiriview::isSupportedDirectVideoFileName(candidate.name)
            || kiriview::isSupportedDirectVideoUrl(candidate.url)
        ? kiriview::ActiveNavigationThumbnailKind::Video
        : kiriview::ActiveNavigationThumbnailKind::Image;
}

kiriview::ActiveNavigationThumbnailKind thumbnailKindForImageDocumentPageTarget(
    const kiriview::ImageDocumentPageTarget &target)
{
    return target.kind == kiriview::ImageDocumentPageKind::Video
        ? kiriview::ActiveNavigationThumbnailKind::Video
        : kiriview::ActiveNavigationThumbnailKind::Image;
}

QString thumbnailLabel(const QString &candidateName, const QUrl &url)
{
    return candidateName.isEmpty() ? url.fileName(QUrl::PrettyDecoded) : candidateName;
}

std::vector<kiriview::ActiveNavigationThumbnailRow> thumbnailRowsForDirectMediaNavigationCandidates(
    const std::vector<kiriview::DirectMediaNavigationCandidate> &candidates, int currentNumber)
{
    std::vector<kiriview::ActiveNavigationThumbnailRow> rows;
    rows.reserve(candidates.size());

    int number = 1;
    for (const kiriview::DirectMediaNavigationCandidate &candidate : candidates) {
        const kiriview::ActiveNavigationThumbnailKind kind
            = thumbnailKindForDirectMediaNavigationCandidate(candidate);
        rows.push_back(kiriview::ActiveNavigationThumbnailRow {
            number,
            candidate.url,
            thumbnailLabel(candidate.name, candidate.url),
            kind,
            kind == kiriview::ActiveNavigationThumbnailKind::Video
                ? kiriview::ActiveNavigationThumbnailSourceKind::DirectVideo
                : kiriview::ActiveNavigationThumbnailSourceKind::DirectImage,
            number == currentNumber,
        });
        ++number;
    }

    return rows;
}

std::vector<kiriview::ActiveNavigationThumbnailRow>
thumbnailRowsForImageDocumentPageNavigationSnapshot(
    const kiriview::ImageDocumentPageNavigationSnapshot &snapshot, int currentNumber)
{
    std::vector<kiriview::ActiveNavigationThumbnailRow> rows;
    rows.reserve(snapshot.state.targets.size());

    int number = 1;
    for (const kiriview::ImageDocumentPageTarget &target : snapshot.state.targets) {
        const kiriview::ActiveNavigationThumbnailKind kind
            = thumbnailKindForImageDocumentPageTarget(target);
        rows.push_back(kiriview::ActiveNavigationThumbnailRow {
            number,
            target.url,
            thumbnailLabel(target.name, target.url),
            kind,
            kind == kiriview::ActiveNavigationThumbnailKind::Video
                ? kiriview::ActiveNavigationThumbnailSourceKind::ImageDocumentPageVideo
                : kiriview::ActiveNavigationThumbnailSourceKind::ImageDocumentPageImage,
            number == currentNumber,
        });
        ++number;
    }

    return rows;
}
}

namespace kiriview {
std::vector<ActiveNavigationThumbnailRow> projectActiveNavigationThumbnailRows(
    ActiveNavigationSourceKind sourceKind, const ActiveNavigationSnapshot &navigation,
    const std::vector<DirectMediaNavigationCandidate> &directMediaNavigationCandidates,
    const ImageDocumentPageNavigationSnapshot &imageDocumentPageNavigationSnapshot)
{
    if (!navigation.available || !navigation.known || navigation.count < 1) {
        return {};
    }

    std::vector<ActiveNavigationThumbnailRow> rows;
    switch (sourceKind) {
    case ActiveNavigationSourceKind::OrdinaryDirectMedia:
        rows = thumbnailRowsForDirectMediaNavigationCandidates(
            directMediaNavigationCandidates, navigation.currentNumber);
        break;
    case ActiveNavigationSourceKind::ImageDocumentPages:
        rows = thumbnailRowsForImageDocumentPageNavigationSnapshot(
            imageDocumentPageNavigationSnapshot, navigation.currentNumber);
        break;
    case ActiveNavigationSourceKind::None:
        break;
    }

    if (static_cast<int>(rows.size()) != navigation.count) {
        return {};
    }

    return rows;
}
}
