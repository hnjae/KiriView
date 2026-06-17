// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTSOURCELOADRUNTIMEPLANEXECUTOR_H
#define KIRIVIEW_IMAGEDOCUMENTSOURCELOADRUNTIMEPLANEXECUTOR_H

#include "imagedocumentruntimeplan.h"

#include <QUrl>
#include <functional>

namespace kiriview {
struct ImageDocumentSourceLoadRuntimeOperations {
    std::function<void()> clearLoadingContainerNavigationUrl;
    std::function<void(const QUrl &)> setLoadingContainerNavigationUrl;
    std::function<void(const QUrl &)> setContainerNavigationUrl;
    std::function<void(const ImageDocumentSourceLoadRequest &)> prepareSourceLoad;
    std::function<void()> beginOpen;
};

class ImageDocumentSourceLoadRuntimePlanExecutor final
{
public:
    explicit ImageDocumentSourceLoadRuntimePlanExecutor(
        ImageDocumentSourceLoadRuntimeOperations operations);

    bool dispatchOperation(const ImageDocumentRuntimeOperation &operation);

private:
    ImageDocumentSourceLoadRuntimeOperations m_operations;
};
}

#endif
