// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_FILEDELETIONFALLBACK_H
#define KIRIVIEW_FILEDELETIONFALLBACK_H

#include "imagecandidaterepository.h"
#include "imagelocation.h"
#include "imagenavigationtypes.h"

#include <QString>
#include <QUrl>
#include <optional>
#include <vector>

namespace KiriView {
enum class DeletionFallbackKind {
    None,
    Image,
    ComicBookArchive,
};

struct DeletionFallbackPlan {
    DeletionFallbackKind kind = DeletionFallbackKind::None;
    std::optional<ImageCandidateListContext> imageContext;
    QUrl currentUrl;
    QString currentName;
    QUrl currentContainerUrl;
};

struct ComicBookDeletionFallbackCandidates {
    std::optional<ContainerNavigationCandidate> preferred;
    std::optional<ContainerNavigationCandidate> fallback;
};

DeletionFallbackPlan deletionFallbackPlanForDisplayedLocation(
    const DisplayedImageLocation &location);
std::optional<QUrl> imageDeletionFallbackUrl(
    std::vector<ImageNavigationCandidate> candidates, const DeletionFallbackPlan &plan);
ComicBookDeletionFallbackCandidates comicBookDeletionFallbackCandidates(
    std::vector<ContainerNavigationCandidate> candidates, const DeletionFallbackPlan &plan);
}

#endif
