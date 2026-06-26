// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodejobstate.h"

#include <utility>

namespace kiriview {
ImageDecodeJobState::ImageDecodeJobState(quint64 nextOperationId)
    : m_operation(nextOperationId)
{
}

ImageDecodeJobTicket ImageDecodeJobState::start(ImageDecodeRequest request)
{
    m_request = std::move(request);
    m_phase = Phase::LoadingData;
    return ImageDecodeJobTicket { m_operation.start(), *m_request };
}

void ImageDecodeJobState::cancel()
{
    m_operation.cancel();
    m_request.reset();
    m_phase = Phase::LoadingData;
}

bool ImageDecodeJobState::hasActiveRequest() const { return m_request.has_value(); }

ImageDecodeJobRuntimePlan ImageDecodeJobState::acceptLoadedData(const ImageDecodeJobTicket& ticket)
{
    if (!accepts(ticket) || m_phase != Phase::LoadingData) {
        return noOperation();
    }

    m_phase = Phase::Decoding;
    return startDecodePlan(*m_request);
}

ImageDecodeJobRuntimePlan ImageDecodeJobState::acceptLoadError(const ImageDecodeJobTicket& ticket)
{
    return claim<DeliverImageLoadErrorOperation>(ticket, Phase::LoadingData);
}

ImageDecodeJobRuntimePlan ImageDecodeJobState::acceptThumbnailPreview(
    const ImageDecodeJobTicket& ticket)
{
    if (!accepts(ticket) || m_phase != Phase::Decoding) {
        return noOperation();
    }

    return thumbnailPreviewPlan(*m_request);
}

ImageDecodeJobRuntimePlan ImageDecodeJobState::acceptDecodeResult(
    const ImageDecodeJobTicket& ticket)
{
    return claim<DeliverImageDecodeResultOperation>(ticket, Phase::Decoding);
}

bool ImageDecodeJobState::accepts(const ImageDecodeJobTicket& ticket) const
{
    return m_request.has_value() && m_operation.accepts(ticket.operationId)
        && m_request->matches(ticket.request);
}

ImageDecodeJobRuntimePlan ImageDecodeJobState::noOperation() const { return {}; }

ImageDecodeJobRuntimePlan ImageDecodeJobState::startDecodePlan(
    const ImageDecodeRequest& request) const
{
    return ImageDecodeJobRuntimePlan {
        StartImageDecodeOperation { request },
    };
}

ImageDecodeJobRuntimePlan ImageDecodeJobState::thumbnailPreviewPlan(
    const ImageDecodeRequest& request) const
{
    return ImageDecodeJobRuntimePlan {
        DeliverImageThumbnailPreviewOperation { request },
    };
}

template <typename Operation>
ImageDecodeJobRuntimePlan ImageDecodeJobState::claim(
    const ImageDecodeJobTicket& ticket, Phase phase)
{
    if (!accepts(ticket) || m_phase != phase) {
        return noOperation();
    }

    ImageDecodeRequest request = std::move(*m_request);
    m_operation.finish(ticket.operationId);
    m_request.reset();
    m_phase = Phase::LoadingData;
    return ImageDecodeJobRuntimePlan {
        Operation { std::move(request) },
    };
}
}
