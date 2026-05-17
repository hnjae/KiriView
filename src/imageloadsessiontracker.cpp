// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloadsessiontracker.h"

#include <utility>

namespace KiriView {
ImageLoadPlan ImageLoadSessionTracker::start(
    ImageLoadRequest request, ImageFirstDisplayDecodeContext firstDisplayContext)
{
    cancel();
    m_firstDisplayContext = firstDisplayContext;

    ImageLoadPlan plan = imageLoadPlan(nextSessionId(), std::move(request));
    m_session = plan.session;
    return plan;
}

void ImageLoadSessionTracker::cancel()
{
    m_session.reset();
    m_firstDisplayContext = {};
}

const ImageFirstDisplayDecodeContext &ImageLoadSessionTracker::firstDisplayContext() const
{
    return m_firstDisplayContext;
}

bool ImageLoadSessionTracker::isCurrent(const ImageLoadSession &session) const
{
    return m_session.has_value() && m_session->id == session.id;
}

std::optional<ImageLoadSession> ImageLoadSessionTracker::currentForDecodeRequest(
    const ImageDecodeRequest &request) const
{
    if (!m_session.has_value() || !request.matches(m_session->id, m_session->location.imageUrl())) {
        return std::nullopt;
    }

    return *m_session;
}

std::optional<ImageLoadSession> ImageLoadSessionTracker::resolveCurrentArchiveImage(
    const ImageLoadSession &session, QUrl imageUrl)
{
    if (!isCurrent(session)) {
        return std::nullopt;
    }

    m_session->location.setImageUrl(std::move(imageUrl));
    return *m_session;
}

std::optional<ImageLoadSession> ImageLoadSessionTracker::replaceCurrentLocation(
    const ImageLoadSession &session, DisplayedImageLocation location)
{
    if (!isCurrent(session)) {
        return std::nullopt;
    }

    m_session->location = std::move(location);
    return *m_session;
}

std::optional<ImageLoadSession> ImageLoadSessionTracker::takeCurrent(
    const ImageLoadSession &session)
{
    if (!isCurrent(session)) {
        return std::nullopt;
    }

    std::optional<ImageLoadSession> currentSession = std::move(m_session);
    m_session.reset();
    return currentSession;
}

quint64 ImageLoadSessionTracker::nextSessionId()
{
    ++m_nextSessionId;
    return m_nextSessionId;
}
}
