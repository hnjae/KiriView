// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentmediaentrysourceruntimeplanexecutor.h"

#include <utility>
#include <variant>

namespace kiriview {
namespace {
    template <typename Operation, typename... Args>
    void run(const Operation &operation, Args &&...args)
    {
        if (operation) {
            operation(std::forward<Args>(args)...);
        }
    }
}

ImageDocumentMediaEntrySourceRuntimePlanExecutor::ImageDocumentMediaEntrySourceRuntimePlanExecutor(
    ImageDocumentMediaEntrySourceRuntimeOperations operations)
    : m_operations(std::move(operations))
{
}

bool ImageDocumentMediaEntrySourceRuntimePlanExecutor::dispatchOperation(
    const ImageDocumentRuntimeOperation &operation)
{
    if (std::holds_alternative<ClearMediaEntrySourceOperation>(operation)) {
        run(m_operations.clear);
        return true;
    }

    return false;
}
}
