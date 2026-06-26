// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEREMOVALFALLBACK_H
#define KIRIVIEW_IMAGEREMOVALFALLBACK_H

#include "location/imagelocation.h"
#include "navigation/imagedocumentpagecandidatelistsource.h"
#include "navigation/imagedocumentpagenavigationtypes.h"

#include <QString>
#include <QUrl>
#include <optional>
#include <variant>
#include <vector>

namespace kiriview {
struct NoImageRemovalFallback
{
};

struct ImageRemovalFallback
{
    ImageDocumentPageCandidateListContext imageContext;
    QUrl currentUrl;
    QString currentName;
};

struct ComicBookRemovalFallback
{
    QUrl currentContainerUrl;
    QUrl candidateDirectoryUrl;
    QString currentName;
};

using ImageRemovalFallbackPlan
    = std::variant<NoImageRemovalFallback, ImageRemovalFallback, ComicBookRemovalFallback>;

struct ComicBookRemovalFallbackCandidates
{
    std::optional<ContainerNavigationCandidate> preferred;
    std::optional<ContainerNavigationCandidate> fallback;
};

struct ImageRemovalPlan
{
    QUrl targetUrl;
    ImageRemovalFallbackPlan fallbackPlan;

    bool hasTarget() const { return !targetUrl.isEmpty(); }
};

ImageRemovalPlan imageRemovalPlanForDisplayedLocation(const DisplayedImageLocation& location);
ImageRemovalFallback imageRemovalFallbackForImageContext(
    const ImageDocumentPageCandidateListContext& context);
std::optional<ImageDocumentPageTarget> imageRemovalFallbackTarget(
    std::vector<ImageDocumentPageCandidate> candidates, const ImageRemovalFallback& fallback);
ComicBookRemovalFallbackCandidates comicBookRemovalFallbackCandidates(
    std::vector<ContainerNavigationCandidate> candidates, const ComicBookRemovalFallback& fallback);
}

#endif
