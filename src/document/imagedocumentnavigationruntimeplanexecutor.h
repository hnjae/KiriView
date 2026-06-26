// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTNAVIGATIONRUNTIMEPLANEXECUTOR_H
#define KIRIVIEW_IMAGEDOCUMENTNAVIGATIONRUNTIMEPLANEXECUTOR_H

#include "imagedocumentruntimeplan.h"

#include <QString>
#include <QUrl>
#include <functional>

namespace kiriview {
struct ImageDocumentNavigationRuntimeOperations
{
    std::function<void()> cancelPageNavigationUpdate;
    std::function<void()> cancelNavigation;
    std::function<void()> cancelContainerNavigation;
    std::function<void()> cancelAllNavigation;
    std::function<void()> clearPageNavigation;
    std::function<void()> updatePageNavigation;
    std::function<void(const ImageDocumentPageTarget&)> loadUrl;
    std::function<void(const ImageDocumentPageTarget&, const QUrl&)> loadContainerImage;
    std::function<void(const QUrl&)> finishEmptyContainerNavigation;
    std::function<void(const QUrl&, const QString&)> finishContainerNavigationLoadWithError;
    std::function<void(NavigationDirection)> reportContainerNavigationBoundary;
    std::function<void(const ContainerNavigationListFailure&)> reportContainerNavigationListFailure;
    std::function<void(const ImageDocumentPageTarget&, bool)> loadPageNavigationUrl;
};

class ImageDocumentNavigationRuntimePlanExecutor final
{
public:
    explicit ImageDocumentNavigationRuntimePlanExecutor(
        ImageDocumentNavigationRuntimeOperations operations);

    bool dispatchOperation(const ImageDocumentRuntimeOperation& operation);

private:
    ImageDocumentNavigationRuntimeOperations m_operations;
};
}

#endif
