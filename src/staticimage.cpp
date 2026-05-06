// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "staticimage.h"

#include "imagebytecost.h"

namespace KiriView {
bool StaticImagePayload::isValid() const { return source != nullptr && !preview.isNull(); }

qsizetype StaticImagePayload::byteCost() const
{
    if (!isValid()) {
        return 0;
    }

    return source->byteCost() + imageByteCost(preview);
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
