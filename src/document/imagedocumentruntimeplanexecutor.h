// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIMEPLANEXECUTOR_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIMEPLANEXECUTOR_H

#include "imagedocumentlifecycleruntimeplanexecutor.h"
#include "imagedocumentmediaentrysourceruntimeplanexecutor.h"
#include "imagedocumentnavigationruntimeplanexecutor.h"
#include "imagedocumentopenruntimeplanexecutor.h"
#include "imagedocumentpredecoderuntimeplanexecutor.h"
#include "imagedocumentruntimeplan.h"
#include "imagedocumentsourceloadruntimeplanexecutor.h"
#include "imagedocumentspreadruntimeplanexecutor.h"

namespace kiriview {
struct ImageDocumentRuntimeOperations {
    ImageDocumentLifecycleRuntimeOperations lifecycle;
    ImageDocumentMediaEntrySourceRuntimeOperations mediaEntrySource;
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
    ImageDocumentLifecycleRuntimePlanExecutor m_lifecycleExecutor;
    ImageDocumentMediaEntrySourceRuntimePlanExecutor m_mediaEntrySourceExecutor;
    ImageDocumentSourceLoadRuntimePlanExecutor m_sourceLoadExecutor;
    ImageDocumentOpenRuntimePlanExecutor m_openExecutor;
    ImageDocumentPredecodeRuntimePlanExecutor m_predecodeExecutor;
    ImageDocumentSpreadRuntimePlanExecutor m_spreadExecutor;
    ImageDocumentNavigationRuntimePlanExecutor m_navigationExecutor;
};
}

#endif
