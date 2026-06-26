// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGE_DOCUMENT_PLAN_TEST_SUPPORT_H
#define KIRIVIEW_IMAGE_DOCUMENT_PLAN_TEST_SUPPORT_H

#include "document/imagedocumentruntimeplan.h"

#include <cstddef>
#include <variant>

namespace kiriview::TestSupport {
template <typename... Operations> struct ImageDocumentRuntimeOperationTypes
{
};

template <typename... Operations>
constexpr ImageDocumentRuntimeOperationTypes<Operations...> operationTypes()
{
    return {};
}

template <typename... Operations>
bool hasOperationTypes(
    const ImageDocumentRuntimePlan& plan, ImageDocumentRuntimeOperationTypes<Operations...>)
{
    if (plan.size() != sizeof...(Operations)) {
        return false;
    }

    std::size_t index = 0;
    return (... && std::holds_alternative<Operations>(plan.at(index++)));
}

template <typename Operation>
const Operation& operationAt(const ImageDocumentRuntimePlan& plan, std::size_t index)
{
    return std::get<Operation>(plan.at(index));
}
}

#endif
