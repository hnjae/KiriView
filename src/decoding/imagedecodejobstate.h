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

enum class ImageDecodeJobRuntimeOperation {
    None,
    StartDecode,
    DeliverLoadError,
    DeliverDecodeResult,
};

struct ImageDecodeJobRuntimePlan {
    ImageDecodeJobRuntimeOperation operation = ImageDecodeJobRuntimeOperation::None;
    ImageDecodeRequest request;

    bool hasOperation() const { return operation != ImageDecodeJobRuntimeOperation::None; }
};

class ImageDecodeJobState final
{
public:
    explicit ImageDecodeJobState(quint64 nextOperationId = 0);

    ImageDecodeJobTicket start(ImageDecodeRequest request);
    void cancel();
    bool hasActiveRequest() const;

    ImageDecodeJobRuntimePlan acceptLoadedData(const ImageDecodeJobTicket &ticket);
    ImageDecodeJobRuntimePlan acceptLoadError(const ImageDecodeJobTicket &ticket);
    ImageDecodeJobRuntimePlan acceptDecodeResult(const ImageDecodeJobTicket &ticket);

private:
    enum class Phase {
        LoadingData,
        Decoding,
    };

    bool accepts(const ImageDecodeJobTicket &ticket) const;
    ImageDecodeJobRuntimePlan noOperation() const;
    ImageDecodeJobRuntimePlan startDecodePlan(const ImageDecodeRequest &request) const;
    ImageDecodeJobRuntimePlan claim(
        const ImageDecodeJobTicket &ticket, Phase phase, ImageDecodeJobRuntimeOperation operation);

    ImageAsyncOperationState m_operation;
    std::optional<ImageDecodeRequest> m_request;
    Phase m_phase = Phase::LoadingData;
};
}

#endif
