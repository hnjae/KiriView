// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/predecodepolicyconversion.h"

#include "bridge/rustqtconversion.h"

namespace KiriView::Bridge {
RustPredecodeDocumentKind rustPredecodeDocumentKind(PredecodeDocumentKind kind)
{
    switch (kind) {
    case PredecodeDocumentKind::Regular:
        return RustPredecodeDocumentKind::Regular;
    case PredecodeDocumentKind::DirectoryDocument:
        return RustPredecodeDocumentKind::DirectoryDocument;
    case PredecodeDocumentKind::ArchiveDocument:
        return RustPredecodeDocumentKind::ArchiveDocument;
    }

    return RustPredecodeDocumentKind::Regular;
}

PredecodeDocumentKind predecodeDocumentKindFromRust(RustPredecodeDocumentKind kind)
{
    switch (kind) {
    case RustPredecodeDocumentKind::Regular:
        return PredecodeDocumentKind::Regular;
    case RustPredecodeDocumentKind::DirectoryDocument:
        return PredecodeDocumentKind::DirectoryDocument;
    case RustPredecodeDocumentKind::ArchiveDocument:
        return PredecodeDocumentKind::ArchiveDocument;
    }

    return PredecodeDocumentKind::Regular;
}

RustPredecodeMomentumMode rustPredecodeMomentumMode(PredecodeMomentumMode mode)
{
    switch (mode) {
    case PredecodeMomentumMode::Neutral:
        return RustPredecodeMomentumMode::Neutral;
    case PredecodeMomentumMode::NextBiased:
        return RustPredecodeMomentumMode::NextBiased;
    case PredecodeMomentumMode::PrevBiased:
        return RustPredecodeMomentumMode::PrevBiased;
    case PredecodeMomentumMode::ScrubbingNext:
        return RustPredecodeMomentumMode::ScrubbingNext;
    case PredecodeMomentumMode::ScrubbingPrev:
        return RustPredecodeMomentumMode::ScrubbingPrev;
    }

    return RustPredecodeMomentumMode::Neutral;
}

PredecodeMomentumMode predecodeMomentumModeFromRust(RustPredecodeMomentumMode mode)
{
    switch (mode) {
    case RustPredecodeMomentumMode::Neutral:
        return PredecodeMomentumMode::Neutral;
    case RustPredecodeMomentumMode::NextBiased:
        return PredecodeMomentumMode::NextBiased;
    case RustPredecodeMomentumMode::PrevBiased:
        return PredecodeMomentumMode::PrevBiased;
    case RustPredecodeMomentumMode::ScrubbingNext:
        return PredecodeMomentumMode::ScrubbingNext;
    case RustPredecodeMomentumMode::ScrubbingPrev:
        return PredecodeMomentumMode::ScrubbingPrev;
    }

    return PredecodeMomentumMode::Neutral;
}

RustPredecodeMomentumDirection rustPredecodeMomentumDirection(PredecodeMomentumDirection direction)
{
    switch (direction) {
    case PredecodeMomentumDirection::None:
        return RustPredecodeMomentumDirection::None;
    case PredecodeMomentumDirection::Previous:
        return RustPredecodeMomentumDirection::Previous;
    case PredecodeMomentumDirection::Next:
        return RustPredecodeMomentumDirection::Next;
    }

    return RustPredecodeMomentumDirection::None;
}

PredecodeMomentumDirection predecodeMomentumDirectionFromRust(
    RustPredecodeMomentumDirection direction)
{
    switch (direction) {
    case RustPredecodeMomentumDirection::None:
        return PredecodeMomentumDirection::None;
    case RustPredecodeMomentumDirection::Previous:
        return PredecodeMomentumDirection::Previous;
    case RustPredecodeMomentumDirection::Next:
        return PredecodeMomentumDirection::Next;
    }

    return PredecodeMomentumDirection::None;
}

RustPredecodePolicyInput rustPredecodePolicyInput(PredecodePolicyInput input)
{
    RustPredecodePolicyInput rustInput {};
    rustInput.document_kind = rustPredecodeDocumentKind(input.documentKind);
    rustInput.momentum_mode = rustPredecodeMomentumMode(input.momentumMode);
    rustInput.power_saver_enabled = input.powerSaverEnabled;
    rustInput.ideal_thread_count = input.idealThreadCount;
    return rustInput;
}

RustPredecodeMomentumState rustPredecodeMomentumState(PredecodeMomentumState state)
{
    return RustPredecodeMomentumState {
        state.lastPageIndex,
        state.lastNavigationMsec,
        state.sameDirectionMoveCount,
        rustPredecodeMomentumDirection(state.lastDirection),
        rustPredecodeMomentumMode(state.mode),
    };
}

PredecodeMomentumState predecodeMomentumStateFromRust(const RustPredecodeMomentumState &state)
{
    return PredecodeMomentumState {
        state.last_page_index,
        state.last_navigation_msec,
        state.same_direction_move_count,
        predecodeMomentumDirectionFromRust(state.last_direction),
        predecodeMomentumModeFromRust(state.mode),
    };
}

RustPredecodeCachedImageState rustPredecodeCachedImageState(const PredecodeCachedImageState &state)
{
    return RustPredecodeCachedImageState {
        state.currentDisplayed,
        state.recentDisplayed,
        state.currentDisplayedPriority,
        state.recentDisplayedPriority,
        state.windowPriority,
        rustByteSize(state.byteCost),
    };
}

RustPredecodeWindowLoadState rustPredecodeWindowLoadState(const PredecodeWindowLoadState &state)
{
    return RustPredecodeWindowLoadState { state.displayed, state.cached, state.inFlight };
}

RustPredecodeQueuedLoadState rustPredecodeQueuedLoadState(const PredecodeQueuedLoadState &state)
{
    return RustPredecodeQueuedLoadState {
        state.valid,
        state.inWindow,
        state.cached,
        state.inFlight,
    };
}

PredecodeSchedulePlan predecodeSchedulePlanFromRust(const RustPredecodeSchedulePlan &plan)
{
    return PredecodeSchedulePlan {
        plan.parallel_limit,
        std::vector<std::size_t>(plan.target_indices.begin(), plan.target_indices.end()),
    };
}

PredecodeQueuedLoadPlan predecodeQueuedLoadPlanFromRust(const RustPredecodeQueuedLoadPlan &plan)
{
    return PredecodeQueuedLoadPlan { plan.found, plan.index, plan.discard_count };
}
}
