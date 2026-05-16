// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEREMOVALFALLBACK_H
#define KIRIVIEW_IMAGEREMOVALFALLBACK_H

#include "imagecandidaterepository.h"
#include "imagelocation.h"
#include "imagenavigationtypes.h"

#include <QString>
#include <QUrl>
#include <optional>
#include <variant>
#include <vector>

namespace KiriView {
struct NoImageRemovalFallback {
};

struct ImageRemovalFallback {
    ImageCandidateListContext imageContext;
    QUrl currentUrl;
    QString currentName;
};

struct ComicBookRemovalFallback {
    QUrl currentContainerUrl;
    QUrl candidateDirectoryUrl;
    QString currentName;
};

using ImageRemovalFallbackPlan
    = std::variant<NoImageRemovalFallback, ImageRemovalFallback, ComicBookRemovalFallback>;

struct ComicBookRemovalFallbackCandidates {
    std::optional<ContainerNavigationCandidate> preferred;
    std::optional<ContainerNavigationCandidate> fallback;
};

ImageRemovalFallbackPlan imageRemovalFallbackPlanForDisplayedLocation(
    const DisplayedImageLocation &location);
ImageRemovalFallback imageRemovalFallbackForImageContext(const ImageCandidateListContext &context);
std::optional<QUrl> imageRemovalFallbackUrl(
    std::vector<ImageNavigationCandidate> candidates, const ImageRemovalFallback &fallback);
ComicBookRemovalFallbackCandidates comicBookRemovalFallbackCandidates(
    std::vector<ContainerNavigationCandidate> candidates, const ComicBookRemovalFallback &fallback);
}

#endif
