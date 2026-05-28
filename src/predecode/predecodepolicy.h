// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODEPOLICY_H
#define KIRIVIEW_PREDECODEPOLICY_H

#include "location/imagelocation.h"

#include <QtGlobal>
#include <cstddef>
#include <optional>
#include <vector>

namespace KiriView {
enum class PredecodeMomentumMode {
    Neutral,
    NextBiased,
    PrevBiased,
    ScrubbingNext,
    ScrubbingPrev,
};

enum class PredecodeMomentumDirection {
    None,
    Previous,
    Next,
};

struct PredecodeSourceProfile {
    std::size_t neutralPreviousImageCount = 1;
    std::size_t neutralNextImageCount = 2;
    std::size_t biasedDirectionImageCount = 3;
    std::size_t parallelLimit = 1;
};

constexpr PredecodeSourceProfile directMediaPredecodeSourceProfile()
{
    return PredecodeSourceProfile { 1, 2, 3, 1 };
}

struct PredecodePolicyInput {
    PredecodeSourceProfile sourceProfile = directMediaPredecodeSourceProfile();
    PredecodeMomentumMode momentumMode = PredecodeMomentumMode::Neutral;
    bool powerSaverEnabled = false;
};

struct PredecodeSchedulePlan {
    std::size_t parallelLimit = 0;
    std::vector<std::size_t> targetIndices;
};

struct PredecodeMomentumState {
    int lastPageIndex = -1;
    qint64 lastNavigationMsec = -1;
    int sameDirectionMoveCount = 0;
    PredecodeMomentumDirection lastDirection = PredecodeMomentumDirection::None;
    PredecodeMomentumMode mode = PredecodeMomentumMode::Neutral;
};

struct PredecodeCachedImageState {
    bool currentDisplayed = false;
    bool recentDisplayed = false;
    std::size_t currentDisplayedPriority = 0;
    std::size_t recentDisplayedPriority = 0;
    std::size_t windowPriority = 0;
    qsizetype byteCost = 0;
};

struct PredecodeWindowLoadState {
    bool displayed = false;
    bool cached = false;
    bool inFlight = false;
};

struct PredecodeQueuedLoadState {
    bool valid = false;
    bool inWindow = false;
    bool cached = false;
    bool inFlight = false;
};

struct PredecodeQueuedLoadPlan {
    bool found = false;
    std::size_t index = 0;
    std::size_t discardCount = 0;
};

int predecodeDebounceMsec();
int predecodeNeutralRefreshMsec();
PredecodeSourceProfile predecodeSourceProfileForOpenedCollectionScope(
    const OpenedCollectionScopeLocation &openedCollectionScope, int idealThreadCount);
std::vector<std::size_t> predecodeRetainedCachedImageIndices(
    const std::vector<PredecodeCachedImageState> &states, std::size_t windowCount,
    qsizetype byteBudget);
std::vector<std::size_t> predecodeMissingWindowLoadIndices(
    const std::vector<PredecodeWindowLoadState> &states);
PredecodeSchedulePlan predecodeSchedulePlan(std::size_t candidateCount,
    std::optional<std::size_t> currentIndex, PredecodePolicyInput input);
PredecodeMomentumState predecodeUpdatedMomentumState(
    PredecodeMomentumState state, int pageIndex, qint64 monotonicMsec);
PredecodeQueuedLoadPlan predecodeNextQueuedLoadPlan(
    const std::vector<PredecodeQueuedLoadState> &states);
}

#endif
