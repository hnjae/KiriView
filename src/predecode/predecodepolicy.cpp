// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodepolicy.h"

#include "bridge/predecodepolicyconversion.h"
#include "bridge/rustqtconversion.h"
#include "kiriview/src/policy/predecodepolicy.cxx.h"

#include <utility>

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

PredecodePolicyInput predecodePolicyInputForImagePageScope(
    const ImagePageScopeLocation &imagePageScope, PredecodeMomentumMode momentumMode,
    bool powerSaverEnabled, int idealThreadCount)
{
    PredecodePolicyInput input {};
    input.momentumMode = momentumMode;
    input.powerSaverEnabled = powerSaverEnabled;
    input.idealThreadCount = idealThreadCount;
    if (imagePageScope.isEmpty()) {
        input.documentKind = PredecodeDocumentKind::Regular;
    } else if (imagePageScope.isDirectory()) {
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
        rustStates.push_back(Bridge::rustPredecodeCachedImageState(state));
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
        rustStates.push_back(Bridge::rustPredecodeWindowLoadState(state));
    }

    rust::Vec<std::size_t> missingIndices
        = rustPredecodeMissingWindowLoadIndices(std::move(rustStates));
    return std::vector<std::size_t>(missingIndices.begin(), missingIndices.end());
}

PredecodeSchedulePlan predecodeSchedulePlan(
    std::size_t candidateCount, std::optional<std::size_t> currentIndex, PredecodePolicyInput input)
{
    const RustPredecodeSchedulePlan rustPlan
        = rustPredecodeSchedulePlan(candidateCount, currentIndex.has_value(),
            currentIndex.value_or(0), Bridge::rustPredecodePolicyInput(input));
    return Bridge::predecodeSchedulePlanFromRust(rustPlan);
}

PredecodeMomentumState predecodeUpdatedMomentumState(
    PredecodeMomentumState state, int pageIndex, qint64 monotonicMsec)
{
    const RustPredecodeMomentumState updated
        = rustPredecodeUpdatedMomentumState(RustPredecodeMomentumUpdateInput {
            Bridge::rustPredecodeMomentumState(state),
            pageIndex,
            monotonicMsec,
        });
    return Bridge::predecodeMomentumStateFromRust(updated);
}

PredecodeQueuedLoadPlan predecodeNextQueuedLoadPlan(
    const std::vector<PredecodeQueuedLoadState> &states)
{
    rust::Vec<RustPredecodeQueuedLoadState> rustStates;
    rustStates.reserve(states.size());
    for (const PredecodeQueuedLoadState &state : states) {
        rustStates.push_back(Bridge::rustPredecodeQueuedLoadState(state));
    }

    const RustPredecodeQueuedLoadPlan rustPlan
        = rustPredecodeNextQueuedLoadPlan(std::move(rustStates));
    return Bridge::predecodeQueuedLoadPlanFromRust(rustPlan);
}
}
