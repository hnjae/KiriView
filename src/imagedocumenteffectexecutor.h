// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTEFFECTEXECUTOR_H
#define KIRIVIEW_IMAGEDOCUMENTEFFECTEXECUTOR_H

#include "imagedocumenteffects.h"

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
    std::function<void()> clearSecondaryPage;
    std::function<void()> notifyRightToLeftReadingChanged;
    std::function<void()> resetZoom;
    std::function<void(const QUrl &)> prepareFailedContainer;
};

struct ImageDocumentNavigationEffectOperations {
    std::function<void()> cancelPageNavigationUpdate;
    std::function<void()> cancelNavigation;
    std::function<void()> cancelContainerNavigation;
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
    std::function<ImageDocumentEffects()> finishEmptySourceLoad;
};

struct ImageDocumentEffectOperations {
    ImageDocumentLifecycleEffectOperations lifecycle;
    ImageDocumentArchiveEffectOperations archive;
    ImageDocumentPredecodeEffectOperations predecode;
    ImageDocumentSpreadEffectOperations spread;
    ImageDocumentNavigationEffectOperations navigation;
    ImageDocumentOpenEffectOperations open;
};

class ImageDocumentEffectExecutor final
{
public:
    explicit ImageDocumentEffectExecutor(ImageDocumentEffectOperations operations);

    void dispatch(ImageDocumentEffect effect);
    void shutdownRuntime();

private:
    void dispatchGeneratedEffects(ImageDocumentEffects effects);
    void clearImage();
    ImageDocumentEffects clearAfterSuccessfulFileDeletion();
    void dispatchPayload(const ClearImageEffect &);
    void dispatchPayload(const ClearDeletedImageEffect &);
    void dispatchPayload(const ResetZoomEffect &);
    void dispatchPayload(const UpdatePageNavigationEffect &);
    void dispatchPayload(const ScheduleAdjacentImagePredecodeEffect &);
    void dispatchPayload(const OpenUrlEffect &payload);
    void dispatchPayload(const ContainerImageSelectedEffect &payload);
    void dispatchPayload(const EmptyContainerSelectedEffect &payload);
    void dispatchPayload(const ContainerNavigationFailedEffect &payload);
    void dispatchPayload(const PageNavigationSelectedEffect &payload);
    void dispatchPayload(const PrepareFailedContainerEffect &payload);

    ImageDocumentEffectOperations m_operations;
};
}

#endif
