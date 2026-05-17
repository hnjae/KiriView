// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTSOURCELOADEXECUTOR_H
#define KIRIVIEW_IMAGEDOCUMENTSOURCELOADEXECUTOR_H

#include "imagedocumentsourceloadpolicy.h"
#include "imagedocumentsourceloadrequest.h"

#include <QUrl>
#include <functional>

namespace KiriView {
struct ImageDocumentSourceLoadOperations {
    std::function<void()> cancelNavigationAndPredecode;
    std::function<void()> finishSpreadTransition;
    std::function<void()> resetRightToLeftReading;
    std::function<void()> notifyRightToLeftReadingChanged;
    std::function<void()> clearSecondaryPage;
    std::function<void()> clearLoadingContainerNavigationUrl;
    std::function<void(const QUrl &)> setLoadingContainerNavigationUrl;
    std::function<void(const QUrl &)> setContainerNavigationUrl;
    std::function<void(const ImageDocumentSourceLoadRequest &)> prepareSourceLoad;
    std::function<void(const QUrl &)> setSourceUrl;
    std::function<void()> beginOpen;
};

void executeImageDocumentSourceLoadPlan(const ImageDocumentSourceLoadRequest &request,
    const ImageDocumentSourceLoadPlan &plan, const ImageDocumentSourceLoadOperations &operations);
}

#endif
