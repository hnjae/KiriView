// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ACTIVENAVIGATIONTHUMBNAILPROJECTION_H
#define KIRIVIEW_ACTIVENAVIGATIONTHUMBNAILPROJECTION_H

#include "navigation/directmedianavigationmodel.h"
#include "navigation/imagedocumentpagenavigationtypes.h"
#include "session/activenavigationprojection.h"

#include <QString>
#include <QUrl>
#include <vector>

namespace kiriview {
enum class ActiveNavigationThumbnailKind {
    Image,
    Video,
};

enum class ActiveNavigationThumbnailSourceKind {
    DirectImage,
    DirectVideo,
    ImageDocumentPageImage,
    ImageDocumentPageVideo,
};

enum class ActiveNavigationThumbnailResultStatus {
    NoResult,
    Pending,
    Ready,
    Unsupported,
    Failed,
};

struct ActiveNavigationThumbnailRow
{
    int number = 0;
    QUrl url;
    QString label;
    ActiveNavigationThumbnailKind kind = ActiveNavigationThumbnailKind::Image;
    ActiveNavigationThumbnailSourceKind sourceKind
        = ActiveNavigationThumbnailSourceKind::DirectImage;
    bool current = false;
};

QString activeNavigationThumbnailPageKindIdentity(ActiveNavigationThumbnailKind kind);
QString activeNavigationThumbnailSourceKindIdentity(ActiveNavigationThumbnailSourceKind sourceKind);
std::vector<ActiveNavigationThumbnailRow> projectActiveNavigationThumbnailRows(
    ActiveNavigationSourceKind sourceKind, ActiveNavigationSnapshot navigation,
    const std::vector<DirectMediaNavigationCandidate>& directMediaNavigationCandidates,
    const ImageDocumentPageNavigationSnapshot& imageDocumentPageNavigationSnapshot);
}

#endif
