// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDECODEJOBSTATE_H
#define KIRIVIEW_IMAGEDECODEJOBSTATE_H

#include "async/imageasyncoperationstate.h"
#include "imagedecoderequest.h"

#include <QtGlobal>
#include <optional>

namespace KiriView {
struct ImageDecodeJobTicket {
    quint64 operationId = 0;
    ImageDecodeRequest request;
};

class ImageDecodeJobState final
{
public:
    explicit ImageDecodeJobState(quint64 nextOperationId = 0);

    ImageDecodeJobTicket start(ImageDecodeRequest request);
    void cancel();
    bool hasActiveRequest() const;

    std::optional<ImageDecodeRequest> beginDecode(const ImageDecodeJobTicket &ticket);
    std::optional<ImageDecodeRequest> claimLoadError(const ImageDecodeJobTicket &ticket);
    std::optional<ImageDecodeRequest> claimDecodeResult(const ImageDecodeJobTicket &ticket);

private:
    enum class Phase {
        LoadingData,
        Decoding,
    };

    bool accepts(const ImageDecodeJobTicket &ticket) const;
    std::optional<ImageDecodeRequest> claim(const ImageDecodeJobTicket &ticket, Phase phase);

    ImageAsyncOperationState m_operation;
    std::optional<ImageDecodeRequest> m_request;
    Phase m_phase = Phase::LoadingData;
};
}

#endif
