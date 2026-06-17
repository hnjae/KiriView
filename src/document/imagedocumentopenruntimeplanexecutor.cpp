// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentopenruntimeplanexecutor.h"

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

ImageDocumentOpenRuntimePlanExecutor::ImageDocumentOpenRuntimePlanExecutor(
    ImageDocumentOpenRuntimeOperations operations)
    : m_operations(std::move(operations))
{
}

bool ImageDocumentOpenRuntimePlanExecutor::dispatchOperation(
    const ImageDocumentRuntimeOperation &operation)
{
    if (std::holds_alternative<CancelOpenOperation>(operation)) {
        run(m_operations.cancelOpen);
        return true;
    }
    if (std::holds_alternative<ClearDisplayedImageLocationOperation>(operation)) {
        run(m_operations.clearDisplayedImageLocation);
        return true;
    }
    if (std::holds_alternative<ClearPresentationImageOperation>(operation)) {
        run(m_operations.clearPresentationImage);
        return true;
    }
    if (const auto *payload = std::get_if<SetSourceUrlOperation>(&operation)) {
        run(m_operations.setSourceUrl, payload->target);
        return true;
    }
    if (const auto *payload = std::get_if<SetErrorStringOperation>(&operation)) {
        run(m_operations.setErrorString, payload->errorString);
        return true;
    }
    if (std::holds_alternative<FinishEmptySourceLoadOperation>(operation)) {
        run(m_operations.finishEmptySourceLoad);
        return true;
    }

    return false;
}
}
