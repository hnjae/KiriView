// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPREDECODERUNTIMEPLANEXECUTOR_H
#define KIRIVIEW_IMAGEDOCUMENTPREDECODERUNTIMEPLANEXECUTOR_H

#include "imagedocumentruntimeplan.h"

#include <functional>

namespace kiriview {
struct ImageDocumentPredecodeRuntimeOperations {
    std::function<void()> clearPredecode;
    std::function<void()> cancelPredecode;
    std::function<void()> scheduleAdjacentImagePredecode;
};

class ImageDocumentPredecodeRuntimePlanExecutor final
{
public:
    explicit ImageDocumentPredecodeRuntimePlanExecutor(
        ImageDocumentPredecodeRuntimeOperations operations);

    bool dispatchOperation(const ImageDocumentRuntimeOperation &operation);

private:
    ImageDocumentPredecodeRuntimeOperations m_operations;
};
}

#endif
