// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTMEDIAENTRYSOURCERUNTIMEPLANEXECUTOR_H
#define KIRIVIEW_IMAGEDOCUMENTMEDIAENTRYSOURCERUNTIMEPLANEXECUTOR_H

#include "imagedocumentruntimeplan.h"

#include <functional>

namespace kiriview {
struct ImageDocumentMediaEntrySourceRuntimeOperations {
    std::function<void()> clear;
};

class ImageDocumentMediaEntrySourceRuntimePlanExecutor final
{
public:
    explicit ImageDocumentMediaEntrySourceRuntimePlanExecutor(
        ImageDocumentMediaEntrySourceRuntimeOperations operations);

    bool dispatchOperation(const ImageDocumentRuntimeOperation &operation);

private:
    ImageDocumentMediaEntrySourceRuntimeOperations m_operations;
};
}

#endif
