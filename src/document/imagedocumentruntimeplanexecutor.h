// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIMEPLANEXECUTOR_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIMEPLANEXECUTOR_H

#include "imagedocumentnavigationruntimeplanexecutor.h"
#include "imagedocumentopenruntimeplanexecutor.h"
#include "imagedocumentpredecoderuntimeplanexecutor.h"
#include "imagedocumentruntimeplan.h"
#include "imagedocumentsourceloadruntimeplanexecutor.h"

#include <QUrl>
#include <functional>

namespace kiriview {
struct ImageDocumentLifecycleRuntimeOperations {
    std::function<void()> cancelFileDeletion;
    std::function<void()> stopPresentationAnimation;
    std::function<void()> shutdownSpread;
};

struct ImageDocumentMediaEntrySourceOperations {
    std::function<void()> clear;
};

struct ImageDocumentSpreadRuntimeOperations {
    std::function<void()> finishSpreadTransition;
    std::function<void()> resetRightToLeftReading;
    std::function<void()> clearSecondaryPage;
    std::function<void()> notifyRightToLeftReadingChanged;
    std::function<void()> resetZoom;
    std::function<void(const QUrl &)> prepareFailedContainer;
};

struct ImageDocumentRuntimeOperations {
    ImageDocumentLifecycleRuntimeOperations lifecycle;
    ImageDocumentMediaEntrySourceOperations mediaEntrySource;
    ImageDocumentPredecodeRuntimeOperations predecode;
    ImageDocumentSpreadRuntimeOperations spread;
    ImageDocumentNavigationRuntimeOperations navigation;
    ImageDocumentOpenRuntimeOperations open;
    ImageDocumentSourceLoadRuntimeOperations sourceLoad;
};

class ImageDocumentRuntimePlanExecutor final
{
public:
    explicit ImageDocumentRuntimePlanExecutor(ImageDocumentRuntimeOperations operations);

    void dispatchPlan(const ImageDocumentRuntimePlan &plan);
    void shutdownRuntime();

private:
    void dispatchOperation(const ImageDocumentRuntimeOperation &operation);

    ImageDocumentRuntimeOperations m_operations;
    ImageDocumentSourceLoadRuntimePlanExecutor m_sourceLoadExecutor;
    ImageDocumentOpenRuntimePlanExecutor m_openExecutor;
    ImageDocumentPredecodeRuntimePlanExecutor m_predecodeExecutor;
    ImageDocumentNavigationRuntimePlanExecutor m_navigationExecutor;
};
}

#endif
