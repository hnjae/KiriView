// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "staticimage.h"

#include "imagebyteaccounting.h"
#include "imagebytecost.h"

namespace KiriView {
FirstDisplayImageDecodeResult ImageTileSource::decodeFirstDisplayImage(
    const ImageFirstDisplayDecodeContext &context, QString *errorString) const
{
    Q_UNUSED(context);
    Q_UNUSED(errorString);
    return {};
}

bool ImageTileSource::isResolutionIndependent() const { return false; }

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
}
