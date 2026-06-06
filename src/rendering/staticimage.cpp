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

bool ImageTileSource::supportsRasterDisplayRefinement() const { return false; }

QImage ImageTileSource::decodeRasterDisplayImage(
    const QSize &rasterSize, QString *errorString) const
{
    Q_UNUSED(rasterSize);
    Q_UNUSED(errorString);
    return {};
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
