// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTSPREADRUNTIMEPLANEXECUTOR_H
#define KIRIVIEW_IMAGEDOCUMENTSPREADRUNTIMEPLANEXECUTOR_H

#include "imagedocumentruntimeplan.h"

#include <QUrl>
#include <functional>

namespace kiriview {
struct ImageDocumentSpreadRuntimeOperations {
    std::function<void()> finishSpreadTransition;
    std::function<void()> resetRightToLeftReading;
    std::function<void()> clearSecondaryPage;
    std::function<void()> notifyRightToLeftReadingChanged;
    std::function<void()> resetZoom;
    std::function<void(const QUrl &)> prepareFailedContainer;
};

class ImageDocumentSpreadRuntimePlanExecutor final
{
public:
    explicit ImageDocumentSpreadRuntimePlanExecutor(
        ImageDocumentSpreadRuntimeOperations operations);

    bool dispatchOperation(const ImageDocumentRuntimeOperation &operation);

private:
    ImageDocumentSpreadRuntimeOperations m_operations;
};
}

#endif
