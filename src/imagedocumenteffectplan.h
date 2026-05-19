// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTEFFECTPLAN_H
#define KIRIVIEW_IMAGEDOCUMENTEFFECTPLAN_H

#include "imagedocumenteffects.h"

#include <QString>
#include <QUrl>
#include <vector>

namespace KiriView {
enum class ImageDocumentRuntimeOperationKind {
    CancelFileDeletion,
    StopPresentationAnimation,
    ShutdownSpread,
    ClearArchiveSession,
    ClearPredecode,
    CancelPredecode,
    ScheduleAdjacentImagePredecode,
    FinishSpreadTransition,
    ClearSecondaryPage,
    NotifyRightToLeftReadingChanged,
    ResetZoom,
    PrepareFailedContainer,
    CancelPageNavigationUpdate,
    CancelNavigation,
    CancelContainerNavigation,
    ClearPageNavigation,
    UpdatePageNavigation,
    LoadUrl,
    LoadContainerImage,
    FinishEmptyContainerNavigation,
    FinishContainerNavigationLoadWithError,
    LoadPageNavigationUrl,
    CancelOpen,
    ClearDisplayedImageLocation,
    ClearPresentationImage,
    SetSourceUrl,
    SetErrorString,
    FinishEmptySourceLoad,
};

struct ImageDocumentRuntimeOperation {
    ImageDocumentRuntimeOperationKind kind = ImageDocumentRuntimeOperationKind::ClearArchiveSession;
    QUrl url;
    QUrl secondaryUrl;
    QString errorString;
    bool preserveTwoPageSpreadTransition = false;

    static ImageDocumentRuntimeOperation simple(ImageDocumentRuntimeOperationKind kind);
    static ImageDocumentRuntimeOperation loadUrl(QUrl url);
    static ImageDocumentRuntimeOperation loadContainerImage(QUrl imageUrl, QUrl containerUrl);
    static ImageDocumentRuntimeOperation finishEmptyContainerNavigation(QUrl containerUrl);
    static ImageDocumentRuntimeOperation finishContainerNavigationLoadWithError(
        QUrl containerUrl, QString errorString);
    static ImageDocumentRuntimeOperation loadPageNavigationUrl(
        QUrl url, bool preserveTwoPageSpreadTransition);
    static ImageDocumentRuntimeOperation prepareFailedContainer(QUrl containerUrl);
    static ImageDocumentRuntimeOperation setSourceUrl(QUrl url);
    static ImageDocumentRuntimeOperation setErrorString(QString errorString);

    bool operator==(const ImageDocumentRuntimeOperation &other) const;
};

using ImageDocumentRuntimePlan = std::vector<ImageDocumentRuntimeOperation>;

ImageDocumentRuntimePlan imageDocumentEffectPlan(const ImageDocumentEffect &effect);
ImageDocumentRuntimePlan imageDocumentShutdownPlan();
}

#endif
