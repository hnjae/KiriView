// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIMEPLANEXECUTOR_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIMEPLANEXECUTOR_H

#include "imagedocumentruntimeplan.h"
#include "imagedocumentsourceloadruntimeplanexecutor.h"

#include <QString>
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

struct ImageDocumentPredecodeRuntimeOperations {
    std::function<void()> clearPredecode;
    std::function<void()> cancelPredecode;
    std::function<void()> scheduleAdjacentImagePredecode;
};

struct ImageDocumentSpreadRuntimeOperations {
    std::function<void()> finishSpreadTransition;
    std::function<void()> resetRightToLeftReading;
    std::function<void()> clearSecondaryPage;
    std::function<void()> notifyRightToLeftReadingChanged;
    std::function<void()> resetZoom;
    std::function<void(const QUrl &)> prepareFailedContainer;
};

struct ImageDocumentNavigationRuntimeOperations {
    std::function<void()> cancelPageNavigationUpdate;
    std::function<void()> cancelNavigation;
    std::function<void()> cancelContainerNavigation;
    std::function<void()> cancelAllNavigation;
    std::function<void()> clearPageNavigation;
    std::function<void()> updatePageNavigation;
    std::function<void(const ImageDocumentPageTarget &)> loadUrl;
    std::function<void(const ImageDocumentPageTarget &, const QUrl &)> loadContainerImage;
    std::function<void(const QUrl &)> finishEmptyContainerNavigation;
    std::function<void(const QUrl &, const QString &)> finishContainerNavigationLoadWithError;
    std::function<void(NavigationDirection)> reportContainerNavigationBoundary;
    std::function<void(const ContainerNavigationListFailure &)>
        reportContainerNavigationListFailure;
    std::function<void(const ImageDocumentPageTarget &, bool)> loadPageNavigationUrl;
};

struct ImageDocumentOpenRuntimeOperations {
    std::function<void()> cancelOpen;
    std::function<void()> clearDisplayedImageLocation;
    std::function<void()> clearPresentationImage;
    std::function<void(const ImageDocumentPageTarget &)> setSourceUrl;
    std::function<void(const QString &)> setErrorString;
    std::function<void()> finishEmptySourceLoad;
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
};
}

#endif
