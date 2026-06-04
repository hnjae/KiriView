// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "staticimage.h"

#include "cache/imagebyteaccounting.h"
#include "cache/imagebytecost.h"

namespace KiriView {
FirstDisplayImageDecodeResult ImageTileSource::decodeFirstDisplayImage(
    const ImageFirstDisplayDecodeContext &context, QString *errorString) const
{
    Q_UNUSED(context);
    Q_UNUSED(errorString);
    return {};
}

bool ImageTileSource::isResolutionIndependent() const { return false; }

StaticImageReaderTransform ImageTileSource::imageReaderTransform() const { return {}; }

bool StaticImagePayload::isValid() const { return source != nullptr && !preview.isNull(); }

qsizetype StaticImagePayload::byteCost() const
{
    if (!isValid()) {
        return 0;
    }

    return saturatedQtByteSum(source->byteCost(), imageByteCost(preview));
}

std::optional<qsizetype> StaticImagePayload::byteCostWithinBudget(qsizetype byteBudget) const
{
    const qsizetype cost = byteCost();
    if (cost <= 0 || cost > byteBudget) {
        return std::nullopt;
    }

    return cost;
}

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

StaticImagePayload StaticDisplayImagePayload::compatibilityStaticImage() const
{
    return StaticImagePayload {
        refinementSource,
        image,
        StaticImageDisplayHints {
            displayPixelsPerSourcePixel,
        },
    };
}
}
