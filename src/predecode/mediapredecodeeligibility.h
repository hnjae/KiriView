// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAPREDECODEELIGIBILITY_H
#define KIRIVIEW_MEDIAPREDECODEELIGIBILITY_H

#include "navigation/directmedianavigationmodel.h"

#include <QUrl>
#include <cstddef>
#include <optional>
#include <vector>

namespace KiriView {
struct MediaPredecodeEligibleImage {
    QUrl url;
    std::size_t mediaIndex = 0;
};

struct MediaPredecodeEligibilitySnapshot {
    std::size_t directMediaNavigationCandidateCount = 0;
    std::optional<std::size_t> currentMediaIndex;
    std::vector<MediaPredecodeEligibleImage> images;
};

MediaPredecodeEligibilitySnapshot mediaPredecodeEligibilitySnapshot(
    const std::vector<DirectMediaNavigationCandidate> &candidates, const QUrl &currentUrl);
std::vector<QUrl> mediaPredecodeEligibleUrlsForTargetIndices(
    const MediaPredecodeEligibilitySnapshot &snapshot, const std::vector<std::size_t> &indices);
}

#endif
