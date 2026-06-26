// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "staticimage.h"

#include "cache/imagebyteaccounting.h"
#include "cache/imagebytecost.h"

namespace kiriview {
namespace {
    void appendDisplayDecodeFailure(ImageTileSourceDisplayDecodeDiagnostics* diagnostics,
        ImageTileSourceDisplayDecodeOperation operation, const QString& userMessage,
        const QString& diagnosticDetail)
    {
        if (diagnostics == nullptr) {
            return;
        }

        diagnostics->failures.push_back(ImageTileSourceDisplayDecodeFailure {
            operation,
            userMessage,
            diagnosticDetail.isEmpty() ? userMessage : diagnosticDetail,
        });
    }
}

QString ImageTileSourceDisplayDecodeDiagnostics::userMessage() const
{
    for (auto failure = failures.crbegin(); failure != failures.crend(); ++failure) {
        if (!failure->userMessage.isEmpty()) {
            return failure->userMessage;
        }
    }
    return {};
}

QString ImageTileSourceDisplayDecodeDiagnostics::diagnosticDetail() const
{
    for (auto failure = failures.crbegin(); failure != failures.crend(); ++failure) {
        if (!failure->diagnosticDetail.isEmpty()) {
            return failure->diagnosticDetail;
        }
    }
    return {};
}

ImageTileSourceFirstDisplayDecodeResult ImageTileSource::decodeFirstDisplayImageWithDiagnostics(
    const ImageFirstDisplayDecodeContext& context) const
{
    QString errorString;
    ImageTileSourceFirstDisplayDecodeResult result;
    result.firstDisplay = decodeFirstDisplayImage(context, &errorString);
    if (result.firstDisplay.status == FirstDisplayImageDecodeStatus::Error) {
        appendDisplayDecodeFailure(&result.diagnostics,
            ImageTileSourceDisplayDecodeOperation::FirstDisplayImage, errorString, errorString);
    }
    return result;
}

FirstDisplayImageDecodeResult ImageTileSource::decodeFirstDisplayImage(
    const ImageFirstDisplayDecodeContext& context, QString* errorString) const
{
    Q_UNUSED(context);
    Q_UNUSED(errorString);
    return {};
}

bool ImageTileSource::supportsRasterDisplayRefinement() const { return false; }

ImageTileSourceDisplayDecodeResult ImageTileSource::decodeRasterDisplayImageWithDiagnostics(
    const QSize& rasterSize) const
{
    if (rasterSize.isEmpty()) {
        return {};
    }

    QString errorString;
    ImageTileSourceDisplayDecodeResult result;
    result.image = decodeRasterDisplayImage(rasterSize, &errorString);
    if (result.image.isNull() && !errorString.isEmpty()) {
        appendDisplayDecodeFailure(&result.diagnostics,
            ImageTileSourceDisplayDecodeOperation::RasterDisplayImage, errorString, errorString);
    }
    return result;
}

QImage ImageTileSource::decodeRasterDisplayImage(
    const QSize& rasterSize, QString* errorString) const
{
    Q_UNUSED(rasterSize);
    Q_UNUSED(errorString);
    return {};
}

ImageTileSourceDisplayDecodeResult ImageTileSource::decodeBlockingDisplayImageWithDiagnostics(
    int maximumLongEdge) const
{
    QString errorString;
    ImageTileSourceDisplayDecodeResult result;
    result.image = decodeBlockingDisplayImage(maximumLongEdge, &errorString);
    if (result.image.isNull() && !errorString.isEmpty()) {
        appendDisplayDecodeFailure(&result.diagnostics,
            ImageTileSourceDisplayDecodeOperation::BlockingDisplayImage, errorString, errorString);
    }
    return result;
}

bool ImageTileSource::isResolutionIndependent() const { return false; }

StaticImageReaderTransform ImageTileSource::imageReaderTransform() const { return {}; }

bool StaticDisplayImagePayload::isValid() const
{
    return !image.isNull() && originalSize.isValid() && !originalSize.isEmpty();
}

qsizetype StaticDisplayImagePayload::byteCost() const
{
    if (!isValid()) {
        return 0;
    }

    const qsizetype sourceCost = refinementSource == nullptr ? 0 : refinementSource->byteCost();
    return saturatedQtByteSum(sourceCost, imageByteCost(image));
}

std::optional<qsizetype> StaticDisplayImagePayload::byteCostWithinBudget(qsizetype byteBudget) const
{
    const qsizetype cost = byteCost();
    if (cost <= 0 || cost > byteBudget) {
        return std::nullopt;
    }

    return cost;
}
}
