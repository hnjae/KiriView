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

namespace KiriView {
enum class ActiveNavigationThumbnailKind {
    Image,
    Video,
};

struct ActiveNavigationThumbnailRow {
    int number = 0;
    QUrl url;
    QString label;
    ActiveNavigationThumbnailKind kind = ActiveNavigationThumbnailKind::Image;
    bool current = false;
};

std::vector<ActiveNavigationThumbnailRow> projectActiveNavigationThumbnailRows(
    ActiveNavigationSourceKind sourceKind, const ActiveNavigationSnapshot &navigation,
    const std::vector<DirectMediaNavigationCandidate> &directMediaNavigationCandidates,
    const ImageDocumentPageNavigationSnapshot &imageDocumentPageNavigationSnapshot);
}

#endif
