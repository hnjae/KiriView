// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEANIMATIONPRESENTERGATE_H
#define KIRIVIEW_IMAGEANIMATIONPRESENTERGATE_H

namespace kiriview {
enum class AnimationPresenterKind {
    ProviderImageRevisions,
    DegradedUnsupported,
};

enum class AnimationProviderChurnGateFailure {
    None,
    FramePacing,
    ProviderRequestLatency,
    StaleFrameRejection,
    MemoryRetention,
    UrlChurn,
};

struct AnimationProviderChurnGateThresholds {
    int maximumFrameScheduleDriftMs = 2;
    int maximumProviderRequestLatencyP95Ms = 2;
    int maximumProviderRequestLatencyMaxMs = 5;
    int maximumPinnedFrameEntriesPerPageRole = 2;
    int providerUrlsPerAcceptedFrame = 1;
};

struct AnimationProviderChurnGateSample {
    int normalizedFrameDelayMs = 0;
    bool timerWaitsForProviderLoad = false;
    int maximumFrameScheduleDriftMs = 0;
    int providerRequestLatencyP95Ms = 0;
    int providerRequestLatencyMaxMs = 0;
    bool staleLoadOutcomeRejected = false;
    int retainedFrameEntriesPerPageRole = 0;
    bool previousFrameRetentionBounded = false;
    int providerUrlsPerAcceptedFrame = 0;
    bool providerUrlsRevisionUnique = false;
};

struct AnimationProviderChurnGateResult {
    AnimationPresenterKind presenter = AnimationPresenterKind::DegradedUnsupported;
    AnimationProviderChurnGateFailure failure = AnimationProviderChurnGateFailure::None;
    bool requiresLoadOutcomeAcknowledgment = false;
    bool requiresPreviousFrameRetention = false;
    int maximumPinnedFrameEntriesPerPageRole = 0;

    bool passed() const;
};

AnimationProviderChurnGateThresholds defaultAnimationProviderChurnGateThresholds();
AnimationProviderChurnGateResult evaluateAnimationProviderChurnGate(
    const AnimationProviderChurnGateSample &sample,
    const AnimationProviderChurnGateThresholds &thresholds
    = defaultAnimationProviderChurnGateThresholds());
}

#endif
