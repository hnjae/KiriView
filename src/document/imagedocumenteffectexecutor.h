// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTEFFECTEXECUTOR_H
#define KIRIVIEW_IMAGEDOCUMENTEFFECTEXECUTOR_H

#include "imagedocumenteffects.h"
#include "imagedocumentruntimeplan.h"

#include <QString>
#include <QUrl>
#include <functional>

namespace KiriView {
struct ImageDocumentLifecycleEffectOperations {
    std::function<void()> cancelFileDeletion;
    std::function<void()> stopPresentationAnimation;
    std::function<void()> shutdownSpread;
};

struct ImageDocumentArchiveEffectOperations {
    std::function<void()> clearSession;
};

struct ImageDocumentPredecodeEffectOperations {
    std::function<void()> clearPredecode;
    std::function<void()> cancelPredecode;
    std::function<void()> scheduleAdjacentImagePredecode;
};

struct ImageDocumentSpreadEffectOperations {
    std::function<void()> finishSpreadTransition;
    std::function<void()> resetRightToLeftReading;
    std::function<void()> clearSecondaryPage;
    std::function<void()> notifyRightToLeftReadingChanged;
    std::function<void()> resetZoom;
    std::function<void(const QUrl &)> prepareFailedContainer;
};

struct ImageDocumentNavigationEffectOperations {
    std::function<void()> cancelPageNavigationUpdate;
    std::function<void()> cancelNavigation;
    std::function<void()> cancelContainerNavigation;
    std::function<void()> cancelAllNavigation;
    std::function<void()> clearPageNavigation;
    std::function<void()> updatePageNavigation;
    std::function<void(const QUrl &)> loadUrl;
    std::function<void(const QUrl &, const QUrl &)> loadContainerImage;
    std::function<void(const QUrl &)> finishEmptyContainerNavigation;
    std::function<void(const QUrl &, const QString &)> finishContainerNavigationLoadWithError;
    std::function<void(const QUrl &, bool)> loadPageNavigationUrl;
};

struct ImageDocumentOpenEffectOperations {
    std::function<void()> cancelOpen;
    std::function<void()> clearDisplayedImageLocation;
    std::function<void()> clearPresentationImage;
    std::function<void(const QUrl &)> setSourceUrl;
    std::function<void(const QString &)> setErrorString;
    std::function<ImageDocumentRuntimePlan()> finishEmptySourceLoad;
};

struct ImageDocumentSourceLoadEffectOperations {
    std::function<void()> clearLoadingContainerNavigationUrl;
    std::function<void(const QUrl &)> setLoadingContainerNavigationUrl;
    std::function<void(const QUrl &)> setContainerNavigationUrl;
    std::function<void(const ImageDocumentSourceLoadRequest &)> prepareSourceLoad;
    std::function<void()> beginOpen;
};

struct ImageDocumentEffectOperations {
    ImageDocumentLifecycleEffectOperations lifecycle;
    ImageDocumentArchiveEffectOperations archive;
    ImageDocumentPredecodeEffectOperations predecode;
    ImageDocumentSpreadEffectOperations spread;
    ImageDocumentNavigationEffectOperations navigation;
    ImageDocumentOpenEffectOperations open;
    ImageDocumentSourceLoadEffectOperations sourceLoad;
};

class ImageDocumentEffectExecutor final
{
public:
    explicit ImageDocumentEffectExecutor(ImageDocumentEffectOperations operations);

    void dispatch(ImageDocumentEffect effect);
    void dispatchPlan(const ImageDocumentRuntimePlan &plan);
    void shutdownRuntime();

private:
    void dispatchOperation(const ImageDocumentRuntimeOperation &operation);

    ImageDocumentEffectOperations m_operations;
};
}

#endif
