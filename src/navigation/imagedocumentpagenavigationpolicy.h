// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPAGENAVIGATIONPOLICY_H
#define KIRIVIEW_IMAGEDOCUMENTPAGENAVIGATIONPOLICY_H

#include "imagedocumentpagenavigationtypes.h"

#include <QUrl>
#include <optional>
#include <vector>

namespace kiriview {
std::vector<QUrl> imageDocumentPageCandidateUrls(
    const std::vector<ImageDocumentPageCandidate>& candidates);
std::vector<ImageDocumentPageTarget> imageDocumentPageCandidateTargets(
    const std::vector<ImageDocumentPageCandidate>& candidates);
std::vector<QUrl> imageDocumentPageTargetUrls(const std::vector<ImageDocumentPageTarget>& targets);
std::vector<QUrl> stillImageDocumentPageCandidateUrls(
    const std::vector<ImageDocumentPageCandidate>& candidates);
bool imageDocumentPageCandidateIsImage(const ImageDocumentPageCandidate& candidate);
bool imageDocumentPageCandidateIsVideo(const ImageDocumentPageCandidate& candidate);
std::optional<std::size_t> imageDocumentPageCandidateIndex(
    const std::vector<ImageDocumentPageCandidate>& candidates, const QUrl& currentUrl);
bool imageDocumentPageCandidatesContainUrl(
    const std::vector<ImageDocumentPageCandidate>& candidates, const QUrl& url);
std::optional<QUrl> adjacentImageDocumentPageUrl(
    const std::vector<ImageDocumentPageCandidate>& candidates, const QUrl& currentUrl,
    NavigationDirection direction);
std::optional<ImageDocumentPageCandidate> adjacentImageDocumentPageCandidate(
    const std::vector<ImageDocumentPageCandidate>& candidates, const QUrl& currentUrl,
    NavigationDirection direction);
std::optional<ContainerNavigationCandidate> adjacentContainerNavigationCandidate(
    const std::vector<ContainerNavigationCandidate>& candidates, const QUrl& currentContainerUrl,
    NavigationDirection direction);
std::optional<std::size_t> containerNavigationCandidateIndex(
    const std::vector<ContainerNavigationCandidate>& candidates, const QUrl& currentContainerUrl);
int pageNavigationCurrentPageNumber(const PageNavigationState& state);
int pageNavigationPageCount(const PageNavigationState& state);
bool pageNavigationHasKnownSelection(const PageNavigationState& state);
std::optional<QUrl> pageNavigationUrlAtPage(const PageNavigationState& state, int pageNumber);
std::optional<ImageDocumentPageTarget> pageNavigationTargetAtPage(
    const PageNavigationState& state, int pageNumber);
std::optional<std::size_t> pageNavigationTargetIndex(
    const PageNavigationState& state, int pageNumber);
std::optional<std::size_t> pageNavigationAdjacentTargetIndex(
    const PageNavigationState& state, NavigationDirection direction);
PageNavigationState pageNavigationStateForCurrentUrl(
    const PageNavigationState& knownState, const QUrl& currentUrl);
PageNavigationState pageNavigationStateForTargets(
    std::vector<ImageDocumentPageTarget> targets, const QUrl& currentUrl);
PageNavigationState pageNavigationStateForUrls(std::vector<QUrl> urls, const QUrl& currentUrl);
bool samePageNavigationState(const PageNavigationState& left, const PageNavigationState& right);
void sortImageDocumentPageCandidates(std::vector<ImageDocumentPageCandidate>* candidates);
void sortContainerNavigationCandidates(std::vector<ContainerNavigationCandidate>* candidates);
}

#endif
