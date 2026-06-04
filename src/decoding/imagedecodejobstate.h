// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDECODEJOBSTATE_H
#define KIRIVIEW_IMAGEDECODEJOBSTATE_H

#include "async/imageasyncoperationstate.h"
#include "imagedecoderequest.h"

#include <QtGlobal>
#include <optional>
#include <variant>

namespace KiriView {
struct ImageDecodeJobTicket {
    quint64 operationId = 0;
    ImageDecodeRequest request;
};

struct NoImageDecodeJobOperation {
};

struct StartImageDecodeOperation {
    ImageDecodeRequest request;
};

struct DeliverImageLoadErrorOperation {
    ImageDecodeRequest request;
};

struct DeliverImageDecodeResultOperation {
    ImageDecodeRequest request;
};
struct DeliverImageThumbnailPreviewOperation {
    ImageDecodeRequest request;
};

using ImageDecodeJobRuntimeOperation = std::variant<NoImageDecodeJobOperation,
    StartImageDecodeOperation, DeliverImageLoadErrorOperation, DeliverImageDecodeResultOperation,
    DeliverImageThumbnailPreviewOperation>;

struct ImageDecodeJobRuntimePlan {
    ImageDecodeJobRuntimeOperation operation;

    bool hasOperation() const
    {
        return !std::holds_alternative<NoImageDecodeJobOperation>(operation);
    }
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
    ImageDecodeJobRuntimePlan acceptThumbnailPreview(const ImageDecodeJobTicket &ticket);
    ImageDecodeJobRuntimePlan acceptDecodeResult(const ImageDecodeJobTicket &ticket);

private:
    enum class Phase {
        LoadingData,
        Decoding,
    };

    bool accepts(const ImageDecodeJobTicket &ticket) const;
    ImageDecodeJobRuntimePlan noOperation() const;
    ImageDecodeJobRuntimePlan startDecodePlan(const ImageDecodeRequest &request) const;
    ImageDecodeJobRuntimePlan thumbnailPreviewPlan(const ImageDecodeRequest &request) const;
    template <typename Operation>
    ImageDecodeJobRuntimePlan claim(const ImageDecodeJobTicket &ticket, Phase phase);

    ImageAsyncOperationState m_operation;
    std::optional<ImageDecodeRequest> m_request;
    Phase m_phase = Phase::LoadingData;
};
}

#endif
