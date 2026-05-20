// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGENAVIGATIONMODEL_H
#define KIRIVIEW_IMAGENAVIGATIONMODEL_H

#include "imagenavigationtypes.h"

#include <QUrl>
#include <optional>
#include <vector>

namespace KiriView {
std::vector<QUrl> imageNavigationCandidateUrls(
    const std::vector<ImageNavigationCandidate> &candidates);
std::optional<std::size_t> imageNavigationCandidateIndex(
    const std::vector<ImageNavigationCandidate> &candidates, const QUrl &currentUrl);
bool imageNavigationCandidatesContainUrl(
    const std::vector<ImageNavigationCandidate> &candidates, const QUrl &url);
std::optional<QUrl> adjacentImageNavigationUrl(
    const std::vector<ImageNavigationCandidate> &candidates, const QUrl &currentUrl,
    NavigationDirection direction);
std::optional<ContainerNavigationCandidate> adjacentContainerNavigationCandidate(
    const std::vector<ContainerNavigationCandidate> &candidates, const QUrl &currentContainerUrl,
    NavigationDirection direction);
int pageNavigationCurrentPageNumber(const PageNavigationState &state);
int pageNavigationImageCount(const PageNavigationState &state);
bool pageNavigationHasKnownSelection(const PageNavigationState &state);
std::optional<QUrl> pageNavigationUrlAtPage(const PageNavigationState &state, int pageNumber);
std::optional<std::size_t> pageNavigationTargetIndex(
    const PageNavigationState &state, int pageNumber);
std::optional<std::size_t> pageNavigationAdjacentTargetIndex(
    const PageNavigationState &state, NavigationDirection direction);
PageNavigationState pageNavigationStateForCurrentUrl(
    const PageNavigationState &knownState, const QUrl &currentUrl);
PageNavigationState pageNavigationStateForUrls(std::vector<QUrl> urls, const QUrl &currentUrl);
bool samePageNavigationState(const PageNavigationState &left, const PageNavigationState &right);
void sortImageNavigationCandidates(std::vector<ImageNavigationCandidate> *candidates);
void sortContainerNavigationCandidates(std::vector<ContainerNavigationCandidate> *candidates);
}

#endif
