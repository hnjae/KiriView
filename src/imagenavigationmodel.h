// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGENAVIGATIONMODEL_H
#define KIRIVIEW_IMAGENAVIGATIONMODEL_H

#include "imagenavigationtypes.h"

#include <KFileItem>
#include <QUrl>
#include <optional>
#include <vector>

namespace KiriView {
std::optional<QUrl> adjacentImageNavigationUrl(
    const std::vector<ImageNavigationCandidate> &candidates, const QUrl &currentUrl,
    NavigationDirection direction);
std::optional<ContainerNavigationCandidate> adjacentContainerNavigationCandidate(
    const std::vector<ContainerNavigationCandidate> &candidates, const QUrl &currentContainerUrl,
    NavigationDirection direction);
std::optional<std::size_t> pageNavigationTargetIndex(
    const PageNavigationState &state, int pageNumber);
std::optional<std::size_t> pageNavigationAdjacentTargetIndex(
    const PageNavigationState &state, NavigationDirection direction);
PageNavigationState pageNavigationStateForCurrentUrl(
    const PageNavigationState &knownState, const QUrl &currentUrl);
PageNavigationState pageNavigationStateForUrls(std::vector<QUrl> urls, const QUrl &currentUrl);
void sortImageNavigationCandidates(std::vector<ImageNavigationCandidate> *candidates);
void sortContainerNavigationCandidates(std::vector<ContainerNavigationCandidate> *candidates);
std::vector<ImageNavigationCandidate> imageNavigationCandidates(const KFileItemList &items);
}

#endif
