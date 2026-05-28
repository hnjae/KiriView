// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DIRECTMEDIANAVIGATIONMODEL_H
#define KIRIVIEW_DIRECTMEDIANAVIGATIONMODEL_H

#include "imagedocumentpagenavigationtypes.h"

#include <QString>
#include <QUrl>
#include <optional>
#include <vector>

namespace KiriView {
struct DirectMediaNavigationCandidate {
    QUrl url;
    QString name;
};

struct DirectMediaNavigationBoundaryState {
    bool canOpenPrevious = false;
    bool canOpenNext = false;
    bool atKnownFirst = false;
    bool atKnownLast = false;
    int currentNumber = 0;
    int count = 0;
};

enum class DirectMediaNavigationOpenKind {
    Previous,
    Next,
    Number,
};

struct DirectMediaNavigationOpenRequest {
    DirectMediaNavigationOpenKind kind = DirectMediaNavigationOpenKind::Next;
    int mediaNumber = 0;
};

struct DirectMediaNavigationOpenPlan {
    DirectMediaNavigationBoundaryState boundaryState;
    std::optional<QUrl> targetUrl;
};

DirectMediaNavigationOpenRequest previousDirectMediaNavigationOpenRequest();
DirectMediaNavigationOpenRequest nextDirectMediaNavigationOpenRequest();
DirectMediaNavigationOpenRequest numberedDirectMediaNavigationOpenRequest(int mediaNumber);
QUrl directMediaNavigationSourceUrl(const QUrl &url);
QUrl directMediaNavigationParentUrl(const QUrl &url);
std::optional<std::size_t> directMediaNavigationCandidateIndex(
    const std::vector<DirectMediaNavigationCandidate> &candidates, const QUrl &currentUrl);
std::optional<QUrl> adjacentDirectMediaNavigationUrl(
    const std::vector<DirectMediaNavigationCandidate> &candidates, const QUrl &currentUrl,
    NavigationDirection direction);
DirectMediaNavigationBoundaryState directMediaNavigationBoundaryState(
    const std::vector<DirectMediaNavigationCandidate> &candidates, const QUrl &currentUrl);
DirectMediaNavigationOpenPlan directMediaNavigationOpenPlan(
    const std::vector<DirectMediaNavigationCandidate> &candidates, const QUrl &currentUrl,
    DirectMediaNavigationOpenRequest request);
void sortDirectMediaNavigationCandidates(std::vector<DirectMediaNavigationCandidate> *candidates);
}

#endif
