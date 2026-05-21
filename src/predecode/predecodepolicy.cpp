// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodepolicy.h"

#include "bridge/rustqtconversion.h"
#include "kiriview/src/policy/predecodecachepolicy.cxx.h"

#include <algorithm>
#include <cstdint>
#include <utility>

namespace {
KiriView::RustPredecodeDocumentKind rustDocumentKind(KiriView::PredecodeDocumentKind kind)
{
    switch (kind) {
    case KiriView::PredecodeDocumentKind::Regular:
        return KiriView::RustPredecodeDocumentKind::Regular;
    case KiriView::PredecodeDocumentKind::DirectoryDocument:
        return KiriView::RustPredecodeDocumentKind::DirectoryDocument;
    case KiriView::PredecodeDocumentKind::ArchiveDocument:
        return KiriView::RustPredecodeDocumentKind::ArchiveDocument;
    }

    return KiriView::RustPredecodeDocumentKind::Regular;
}

KiriView::RustPredecodeMomentumMode rustMomentumMode(KiriView::PredecodeMomentumMode mode)
{
    switch (mode) {
    case KiriView::PredecodeMomentumMode::Neutral:
        return KiriView::RustPredecodeMomentumMode::Neutral;
    case KiriView::PredecodeMomentumMode::NextBiased:
        return KiriView::RustPredecodeMomentumMode::NextBiased;
    case KiriView::PredecodeMomentumMode::PrevBiased:
        return KiriView::RustPredecodeMomentumMode::PrevBiased;
    case KiriView::PredecodeMomentumMode::ScrubbingNext:
        return KiriView::RustPredecodeMomentumMode::ScrubbingNext;
    case KiriView::PredecodeMomentumMode::ScrubbingPrev:
        return KiriView::RustPredecodeMomentumMode::ScrubbingPrev;
    }

    return KiriView::RustPredecodeMomentumMode::Neutral;
}

KiriView::PredecodeMomentumMode momentumMode(KiriView::RustPredecodeMomentumMode mode)
{
    switch (mode) {
    case KiriView::RustPredecodeMomentumMode::Neutral:
        return KiriView::PredecodeMomentumMode::Neutral;
    case KiriView::RustPredecodeMomentumMode::NextBiased:
        return KiriView::PredecodeMomentumMode::NextBiased;
    case KiriView::RustPredecodeMomentumMode::PrevBiased:
        return KiriView::PredecodeMomentumMode::PrevBiased;
    case KiriView::RustPredecodeMomentumMode::ScrubbingNext:
        return KiriView::PredecodeMomentumMode::ScrubbingNext;
    case KiriView::RustPredecodeMomentumMode::ScrubbingPrev:
        return KiriView::PredecodeMomentumMode::ScrubbingPrev;
    }

    return KiriView::PredecodeMomentumMode::Neutral;
}

KiriView::RustPredecodeMomentumDirection rustMomentumDirection(
    KiriView::PredecodeMomentumDirection direction)
{
    switch (direction) {
    case KiriView::PredecodeMomentumDirection::None:
        return KiriView::RustPredecodeMomentumDirection::None;
    case KiriView::PredecodeMomentumDirection::Previous:
        return KiriView::RustPredecodeMomentumDirection::Previous;
    case KiriView::PredecodeMomentumDirection::Next:
        return KiriView::RustPredecodeMomentumDirection::Next;
    }

    return KiriView::RustPredecodeMomentumDirection::None;
}

KiriView::PredecodeMomentumDirection momentumDirection(
    KiriView::RustPredecodeMomentumDirection direction)
{
    switch (direction) {
    case KiriView::RustPredecodeMomentumDirection::None:
        return KiriView::PredecodeMomentumDirection::None;
    case KiriView::RustPredecodeMomentumDirection::Previous:
        return KiriView::PredecodeMomentumDirection::Previous;
    case KiriView::RustPredecodeMomentumDirection::Next:
        return KiriView::PredecodeMomentumDirection::Next;
    }

    return KiriView::PredecodeMomentumDirection::None;
}

KiriView::RustPredecodePolicyInput rustPolicyInput(KiriView::PredecodePolicyInput input)
{
    KiriView::RustPredecodePolicyInput rustInput {};
    rustInput.document_kind = rustDocumentKind(input.documentKind);
    rustInput.momentum_mode = rustMomentumMode(input.momentumMode);
    rustInput.power_saver_enabled = input.powerSaverEnabled;
    rustInput.ideal_thread_count = input.idealThreadCount;
    return rustInput;
}

}

namespace KiriView {
qsizetype predecodePreferredByteBudget()
{
    return Bridge::qtByteSize(rustPredecodePreferredByteBudget());
}

int predecodeDebounceMsec() { return rustPredecodeDebounceMsec(); }

int predecodeNeutralRefreshMsec() { return rustPredecodeNeutralRefreshMsec(); }

qsizetype predecodeByteBudgetForSystemMemory(qsizetype systemMemoryByteSize)
{
    return Bridge::qtByteSize(
        rustPredecodeByteBudgetForSystemMemory(Bridge::rustByteSize(systemMemoryByteSize)));
}

PredecodePolicyInput predecodePolicyInputForArchiveDocument(
    const ArchiveDocumentLocation &archiveDocument, PredecodeMomentumMode momentumMode,
    bool powerSaverEnabled, int idealThreadCount)
{
    PredecodePolicyInput input {};
    input.momentumMode = momentumMode;
    input.powerSaverEnabled = powerSaverEnabled;
    input.idealThreadCount = idealThreadCount;
    if (archiveDocument.isEmpty()) {
        input.documentKind = PredecodeDocumentKind::Regular;
    } else if (archiveDocument.isDirectory()) {
        input.documentKind = PredecodeDocumentKind::DirectoryDocument;
    } else {
        input.documentKind = PredecodeDocumentKind::ArchiveDocument;
    }
    return input;
}

std::vector<std::size_t> predecodeRetainedCachedImageIndices(
    const std::vector<PredecodeCachedImageState> &states, std::size_t windowCount,
    qsizetype byteBudget)
{
    rust::Vec<RustPredecodeCachedImageState> rustStates;
    rustStates.reserve(states.size());
    for (const PredecodeCachedImageState &state : states) {
        rustStates.push_back(RustPredecodeCachedImageState {
            state.currentDisplayed,
            state.recentDisplayed,
            state.currentDisplayedPriority,
            state.recentDisplayedPriority,
            state.windowPriority,
            Bridge::rustByteSize(state.byteCost),
        });
    }

    rust::Vec<std::size_t> retainedIndices = rustPredecodeRetainedCachedImageIndices(
        std::move(rustStates), windowCount, Bridge::rustByteSize(byteBudget));
    return std::vector<std::size_t>(retainedIndices.begin(), retainedIndices.end());
}

std::vector<std::size_t> predecodeMissingWindowLoadIndices(
    const std::vector<PredecodeWindowLoadState> &states)
{
    rust::Vec<RustPredecodeWindowLoadState> rustStates;
    rustStates.reserve(states.size());
    for (const PredecodeWindowLoadState &state : states) {
        rustStates.push_back(
            RustPredecodeWindowLoadState { state.displayed, state.cached, state.inFlight });
    }

    rust::Vec<std::size_t> missingIndices
        = rustPredecodeMissingWindowLoadIndices(std::move(rustStates));
    return std::vector<std::size_t>(missingIndices.begin(), missingIndices.end());
}

PredecodeSchedulePlan predecodeSchedulePlan(
    std::size_t candidateCount, std::optional<std::size_t> currentIndex, PredecodePolicyInput input)
{
    const RustPredecodeSchedulePlan rustPlan = rustPredecodeSchedulePlan(
        candidateCount, currentIndex.has_value(), currentIndex.value_or(0), rustPolicyInput(input));
    return PredecodeSchedulePlan {
        rustPlan.parallel_limit,
        std::vector<std::size_t>(rustPlan.target_indices.begin(), rustPlan.target_indices.end()),
    };
}

PredecodeMomentumState predecodeUpdatedMomentumState(
    PredecodeMomentumState state, int pageIndex, qint64 monotonicMsec)
{
    const RustPredecodeMomentumState updated
        = rustPredecodeUpdatedMomentumState(RustPredecodeMomentumUpdateInput {
            RustPredecodeMomentumState {
                state.lastPageIndex,
                state.lastNavigationMsec,
                state.sameDirectionMoveCount,
                rustMomentumDirection(state.lastDirection),
                rustMomentumMode(state.mode),
            },
            pageIndex,
            monotonicMsec,
        });

    return PredecodeMomentumState {
        updated.last_page_index,
        updated.last_navigation_msec,
        updated.same_direction_move_count,
        momentumDirection(updated.last_direction),
        momentumMode(updated.mode),
    };
}

PredecodeQueuedLoadPlan predecodeNextQueuedLoadPlan(
    const std::vector<PredecodeQueuedLoadState> &states)
{
    rust::Vec<RustPredecodeQueuedLoadState> rustStates;
    rustStates.reserve(states.size());
    for (const PredecodeQueuedLoadState &state : states) {
        rustStates.push_back(RustPredecodeQueuedLoadState {
            state.valid,
            state.inWindow,
            state.cached,
            state.inFlight,
        });
    }

    const RustPredecodeQueuedLoadPlan rustPlan
        = rustPredecodeNextQueuedLoadPlan(std::move(rustStates));
    return PredecodeQueuedLoadPlan { rustPlan.found, rustPlan.index, rustPlan.discard_count };
}
}
