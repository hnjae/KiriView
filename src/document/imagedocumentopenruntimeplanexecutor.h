// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTOPENRUNTIMEPLANEXECUTOR_H
#define KIRIVIEW_IMAGEDOCUMENTOPENRUNTIMEPLANEXECUTOR_H

#include "imagedocumentruntimeplan.h"

#include <QString>
#include <functional>

namespace kiriview {
struct ImageDocumentOpenRuntimeOperations
{
    std::function<void()> cancelOpen;
    std::function<void()> clearDisplayedImageLocation;
    std::function<void()> clearPresentationImage;
    std::function<void(const ImageDocumentPageTarget&)> setSourceUrl;
    std::function<void(const QString&)> setErrorString;
    std::function<void()> finishEmptySourceLoad;
};

class ImageDocumentOpenRuntimePlanExecutor final
{
public:
    explicit ImageDocumentOpenRuntimePlanExecutor(ImageDocumentOpenRuntimeOperations operations);

    bool dispatchOperation(const ImageDocumentRuntimeOperation& operation);

private:
    ImageDocumentOpenRuntimeOperations m_operations;
};
}

#endif
