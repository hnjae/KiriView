// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageanimationpresentergate.h"

namespace KiriView {
bool AnimationProviderChurnGateResult::passed() const
{
    return presenter == AnimationPresenterKind::ProviderImageRevisions
        && failure == AnimationProviderChurnGateFailure::None;
}

AnimationProviderChurnGateThresholds defaultAnimationProviderChurnGateThresholds() { return {}; }

AnimationProviderChurnGateResult evaluateAnimationProviderChurnGate(
    const AnimationProviderChurnGateSample &sample,
    const AnimationProviderChurnGateThresholds &thresholds)
{
    AnimationProviderChurnGateResult result;
    result.maximumPinnedFrameEntriesPerPageRole = thresholds.maximumPinnedFrameEntriesPerPageRole;

    if (sample.normalizedFrameDelayMs <= 0 || sample.timerWaitsForProviderLoad
        || sample.maximumFrameScheduleDriftMs > thresholds.maximumFrameScheduleDriftMs) {
        result.failure = AnimationProviderChurnGateFailure::FramePacing;
        return result;
    }

    if (sample.providerRequestLatencyP95Ms < 0 || sample.providerRequestLatencyMaxMs < 0
        || sample.providerRequestLatencyP95Ms > thresholds.maximumProviderRequestLatencyP95Ms
        || sample.providerRequestLatencyMaxMs > thresholds.maximumProviderRequestLatencyMaxMs) {
        result.failure = AnimationProviderChurnGateFailure::ProviderRequestLatency;
        return result;
    }

    if (!sample.staleLoadOutcomeRejected) {
        result.failure = AnimationProviderChurnGateFailure::StaleFrameRejection;
        return result;
    }

    if (sample.retainedFrameEntriesPerPageRole < 0
        || sample.retainedFrameEntriesPerPageRole > thresholds.maximumPinnedFrameEntriesPerPageRole
        || !sample.previousFrameRetentionBounded) {
        result.failure = AnimationProviderChurnGateFailure::MemoryRetention;
        return result;
    }

    if (sample.providerUrlsPerAcceptedFrame != thresholds.providerUrlsPerAcceptedFrame
        || !sample.providerUrlsRevisionUnique) {
        result.failure = AnimationProviderChurnGateFailure::UrlChurn;
        return result;
    }

    result.presenter = AnimationPresenterKind::ProviderImageRevisions;
    result.requiresLoadOutcomeAcknowledgment = true;
    result.requiresPreviousFrameRetention = true;
    result.failure = AnimationProviderChurnGateFailure::None;
    return result;
}
}
