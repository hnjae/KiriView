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
#include <variant>
#include <vector>

namespace KiriView {
struct NoDeletionFallbackPlan {
};

struct ImageDeletionFallbackPlan {
    ImageCandidateListContext imageContext;
    QUrl currentUrl;
    QString currentName;
};

struct ComicBookDeletionFallbackPlan {
    QUrl currentContainerUrl;
    QUrl candidateDirectoryUrl;
    QString currentName;
};

using DeletionFallbackPlan = std::variant<NoDeletionFallbackPlan, ImageDeletionFallbackPlan,
    ComicBookDeletionFallbackPlan>;

struct ComicBookDeletionFallbackCandidates {
    std::optional<ContainerNavigationCandidate> preferred;
    std::optional<ContainerNavigationCandidate> fallback;
};

DeletionFallbackPlan deletionFallbackPlanForDisplayedLocation(
    const DisplayedImageLocation &location);
std::optional<QUrl> imageDeletionFallbackUrl(
    std::vector<ImageNavigationCandidate> candidates, const ImageDeletionFallbackPlan &plan);
ComicBookDeletionFallbackCandidates comicBookDeletionFallbackCandidates(
    std::vector<ContainerNavigationCandidate> candidates,
    const ComicBookDeletionFallbackPlan &plan);
}

#endif
