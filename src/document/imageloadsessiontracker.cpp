// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloadsessiontracker.h"

#include <utility>

namespace kiriview {
ImageLoadSessionTracker::ImageLoadSessionTracker(quint64 nextSessionId)
    : m_sessionIds(nextSessionId)
{
}

ImageLoadPlan ImageLoadSessionTracker::start(
    ImageLoadRequest request, ImageFirstDisplayDecodeContext firstDisplayContext)
{
    cancel();

    ImageLoadPlan plan = imageLoadPlan(nextSessionId(), std::move(request), firstDisplayContext);
    m_session = plan.session;
    return plan;
}

void ImageLoadSessionTracker::cancel() { m_session.reset(); }

bool ImageLoadSessionTracker::isCurrent(const ImageLoadSession &session) const
{
    return m_session.has_value() && m_session->sameSession(session);
}

std::optional<ImageLoadSession> ImageLoadSessionTracker::currentForDecodeRequest(
    const ImageDecodeRequest &request) const
{
    if (!m_session.has_value() || !request.matches(m_session->decodeRequest())) {
        return std::nullopt;
    }

    return *m_session;
}

std::optional<ImageLoadSession> ImageLoadSessionTracker::claimCurrentForDecodeRequest(
    const ImageDecodeRequest &request)
{
    if (!m_session.has_value() || !request.matches(m_session->decodeRequest())) {
        return std::nullopt;
    }

    std::optional<ImageLoadSession> currentSession = std::move(m_session);
    m_session.reset();
    return currentSession;
}

OpenedCollectionCandidateCompletion ImageLoadSessionTracker::completeOpenedCollectionCandidates(
    const ImageLoadSession &session, const std::vector<ImageDocumentPageCandidate> &candidates)
{
    if (!isCurrent(session)) {
        return {};
    }

    if (candidates.empty()) {
        std::optional<ImageLoadSession> claimedSession = claimCurrent(session);
        return OpenedCollectionCandidateCompletion {
            OpenedCollectionCandidateCompletionAction::ReportEmptyOpenedCollection,
            claimedSession.value_or(ImageLoadSession {}),
        };
    }

    m_session->setImageDocumentPageCandidate(candidates.front());
    if (candidates.front().kind == ImageDocumentPageKind::Video) {
        std::optional<ImageLoadSession> claimedSession = claimCurrent(*m_session);
        return OpenedCollectionCandidateCompletion {
            OpenedCollectionCandidateCompletionAction::ReportUnsupportedOpenedCollectionVideo,
            claimedSession.value_or(ImageLoadSession {}),
        };
    }

    return OpenedCollectionCandidateCompletion {
        OpenedCollectionCandidateCompletionAction::StartImageDecode,
        *m_session,
    };
}

std::optional<ImageLoadSession> ImageLoadSessionTracker::claimPredecodedImage(
    const ImageLoadSession &session, DisplayedImageLocation location)
{
    if (!isCurrent(session)) {
        return std::nullopt;
    }

    m_session->setLocation(std::move(location));
    return claimCurrent(*m_session);
}

std::optional<ImageLoadSession> ImageLoadSessionTracker::claimCurrent(
    const ImageLoadSession &session)
{
    if (!isCurrent(session)) {
        return std::nullopt;
    }

    std::optional<ImageLoadSession> currentSession = std::move(m_session);
    m_session.reset();
    return currentSession;
}

quint64 ImageLoadSessionTracker::nextSessionId() { return m_sessionIds.next(); }
}
