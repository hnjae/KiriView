// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTLIFECYCLERUNTIMEPLANEXECUTOR_H
#define KIRIVIEW_IMAGEDOCUMENTLIFECYCLERUNTIMEPLANEXECUTOR_H

#include "imagedocumentruntimeplan.h"

#include <functional>

namespace kiriview {
struct ImageDocumentLifecycleRuntimeOperations {
    std::function<void()> cancelFileDeletion;
    std::function<void()> stopPresentationAnimation;
    std::function<void()> shutdownSpread;
};

class ImageDocumentLifecycleRuntimePlanExecutor final
{
public:
    explicit ImageDocumentLifecycleRuntimePlanExecutor(
        ImageDocumentLifecycleRuntimeOperations operations);

    bool dispatchOperation(const ImageDocumentRuntimeOperation &operation);

private:
    ImageDocumentLifecycleRuntimeOperations m_operations;
};
}

#endif
