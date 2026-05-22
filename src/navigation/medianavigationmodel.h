// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIANAVIGATIONMODEL_H
#define KIRIVIEW_MEDIANAVIGATIONMODEL_H

#include "imagenavigationtypes.h"

#include <QString>
#include <QUrl>
#include <optional>
#include <vector>

namespace KiriView {
struct MediaNavigationCandidate {
    QUrl url;
    QString name;
};

struct MediaNavigationBoundaryState {
    bool canOpenPrevious = false;
    bool canOpenNext = false;
    bool atKnownFirst = false;
    bool atKnownLast = false;
    int currentNumber = 0;
    int count = 0;
};

struct MediaDeletionFallbackPlan {
    QUrl targetUrl;
    std::optional<QUrl> preferredFallbackUrl;
    std::optional<QUrl> fallbackUrl;

    bool hasTarget() const { return !targetUrl.isEmpty(); }
};

enum class MediaNavigationOpenKind {
    Previous,
    Next,
    Number,
};

struct MediaNavigationOpenRequest {
    MediaNavigationOpenKind kind = MediaNavigationOpenKind::Next;
    int mediaNumber = 0;
};

struct MediaNavigationOpenPlan {
    MediaNavigationBoundaryState boundaryState;
    std::optional<QUrl> targetUrl;
};

MediaNavigationOpenRequest previousMediaNavigationOpenRequest();
MediaNavigationOpenRequest nextMediaNavigationOpenRequest();
MediaNavigationOpenRequest numberedMediaNavigationOpenRequest(int mediaNumber);
QUrl mediaNavigationSourceUrl(const QUrl &url);
QUrl mediaNavigationParentUrl(const QUrl &url);
std::optional<std::size_t> mediaNavigationCandidateIndex(
    const std::vector<MediaNavigationCandidate> &candidates, const QUrl &currentUrl);
std::optional<QUrl> adjacentMediaNavigationUrl(
    const std::vector<MediaNavigationCandidate> &candidates, const QUrl &currentUrl,
    NavigationDirection direction);
MediaNavigationBoundaryState mediaNavigationBoundaryState(
    const std::vector<MediaNavigationCandidate> &candidates, const QUrl &currentUrl);
MediaNavigationOpenPlan mediaNavigationOpenPlan(
    const std::vector<MediaNavigationCandidate> &candidates, const QUrl &currentUrl,
    MediaNavigationOpenRequest request);
MediaDeletionFallbackPlan mediaDeletionFallbackPlan(
    std::vector<MediaNavigationCandidate> candidates, const QUrl &currentUrl);
void sortMediaNavigationCandidates(std::vector<MediaNavigationCandidate> *candidates);
}

#endif
