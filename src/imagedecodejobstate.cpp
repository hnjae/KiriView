// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodejobstate.h"

#include <utility>

namespace KiriView {
ImageDecodeJobTicket ImageDecodeJobState::start(ImageDecodeRequest request)
{
    m_request = std::move(request);
    m_phase = Phase::LoadingData;
    return ImageDecodeJobTicket { m_generation.next(), *m_request };
}

void ImageDecodeJobState::cancel()
{
    m_generation.invalidate();
    m_request.reset();
    m_phase = Phase::LoadingData;
}

bool ImageDecodeJobState::hasActiveRequest() const { return m_request.has_value(); }

std::optional<ImageDecodeRequest> ImageDecodeJobState::beginDecode(
    const ImageDecodeJobTicket &ticket)
{
    if (!accepts(ticket) || m_phase != Phase::LoadingData) {
        return std::nullopt;
    }

    m_phase = Phase::Decoding;
    return *m_request;
}

std::optional<ImageDecodeRequest> ImageDecodeJobState::claimLoadError(
    const ImageDecodeJobTicket &ticket)
{
    return claim(ticket, Phase::LoadingData);
}

std::optional<ImageDecodeRequest> ImageDecodeJobState::claimDecodeResult(
    const ImageDecodeJobTicket &ticket)
{
    return claim(ticket, Phase::Decoding);
}

bool ImageDecodeJobState::accepts(const ImageDecodeJobTicket &ticket) const
{
    return m_request.has_value() && m_generation.accepts(ticket.generation)
        && m_request->matches(ticket.request);
}

std::optional<ImageDecodeRequest> ImageDecodeJobState::claim(
    const ImageDecodeJobTicket &ticket, Phase phase)
{
    if (!accepts(ticket) || m_phase != phase) {
        return std::nullopt;
    }

    std::optional<ImageDecodeRequest> request = std::move(m_request);
    m_request.reset();
    m_phase = Phase::LoadingData;
    return request;
}
}
