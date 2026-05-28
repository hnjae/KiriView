// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "activenavigationthumbnailprojection.h"

#include "navigation/mediaformatregistry.h"

#include <cstddef>

namespace {
KiriView::ActiveNavigationThumbnailKind thumbnailKindForDirectMediaNavigationCandidate(
    const KiriView::DirectMediaNavigationCandidate &candidate)
{
    return KiriView::isSupportedDirectVideoFileName(candidate.name)
            || KiriView::isSupportedDirectVideoUrl(candidate.url)
        ? KiriView::ActiveNavigationThumbnailKind::Video
        : KiriView::ActiveNavigationThumbnailKind::Image;
}

KiriView::ActiveNavigationThumbnailKind thumbnailKindForImageDocumentPageTarget(
    const KiriView::ImageDocumentPageTarget &target)
{
    return target.kind == KiriView::ImageDocumentPageKind::Video
        ? KiriView::ActiveNavigationThumbnailKind::Video
        : KiriView::ActiveNavigationThumbnailKind::Image;
}

QString thumbnailLabel(const QString &candidateName, const QUrl &url)
{
    return candidateName.isEmpty() ? url.fileName(QUrl::PrettyDecoded) : candidateName;
}

std::vector<KiriView::ActiveNavigationThumbnailRow> thumbnailRowsForDirectMediaNavigationCandidates(
    const std::vector<KiriView::DirectMediaNavigationCandidate> &candidates, int currentNumber)
{
    std::vector<KiriView::ActiveNavigationThumbnailRow> rows;
    rows.reserve(candidates.size());

    int number = 1;
    for (const KiriView::DirectMediaNavigationCandidate &candidate : candidates) {
        rows.push_back(KiriView::ActiveNavigationThumbnailRow {
            number,
            candidate.url,
            thumbnailLabel(candidate.name, candidate.url),
            thumbnailKindForDirectMediaNavigationCandidate(candidate),
            number == currentNumber,
        });
        ++number;
    }

    return rows;
}

std::vector<KiriView::ActiveNavigationThumbnailRow>
thumbnailRowsForImageDocumentPageNavigationSnapshot(
    const KiriView::ImageDocumentPageNavigationSnapshot &snapshot, int currentNumber)
{
    std::vector<KiriView::ActiveNavigationThumbnailRow> rows;
    rows.reserve(snapshot.state.targets.size());

    int number = 1;
    for (const KiriView::ImageDocumentPageTarget &target : snapshot.state.targets) {
        rows.push_back(KiriView::ActiveNavigationThumbnailRow {
            number,
            target.url,
            thumbnailLabel(target.name, target.url),
            thumbnailKindForImageDocumentPageTarget(target),
            number == currentNumber,
        });
        ++number;
    }

    return rows;
}
}

namespace KiriView {
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
