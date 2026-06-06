// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENAPPLICATIONPLAN_H
#define KIRIVIEW_IMAGEOPENAPPLICATIONPLAN_H

#include "imagedocumentruntimeplan.h"
#include "imagedocumenttypes.h"
#include "imageloadtypes.h"
#include "location/imagelocation.h"

#include <QString>
#include <QUrl>
#include <optional>

namespace KiriView {
struct ImageOpenResolvedStateDelta {
    std::optional<QUrl> sourceUrl;
    std::optional<ImageDocumentPageKind> sourceKind;
    std::optional<DisplayedImageLocation> displayedLocation;
    std::optional<QUrl> containerNavigationUrl;
    std::optional<bool> loading;
    std::optional<ImageDocumentStatus> status;
    std::optional<QString> errorString;
    std::optional<bool> unsupportedOpenedCollectionVideo;
    bool clearEmbeddedMetadata = false;
    bool clearLoadingContainerNavigationUrl = false;
};

struct ImageOpenApplicationPlan {
    ImageOpenResolvedStateDelta stateDelta;
    ImageDocumentRuntimePlan runtimePlan;
};

struct ImageOpenTransitionContext {
    const ImageLoadSession *session = nullptr;
    std::optional<QUrl> containerUrl;
    std::optional<QUrl> displayedUrl;
    std::optional<QString> errorString;

    static ImageOpenTransitionContext sourceResolved(const ImageLoadSession &session);
    static ImageOpenTransitionContext successfulImageLoad(const ImageLoadSession &session);
    static ImageOpenTransitionContext sourceLoadError(
        const ImageLoadSession &session, const QUrl &displayedUrl, const QString &errorString);
    static ImageOpenTransitionContext containerNavigationError(
        const QUrl &containerUrl, const QString &errorString);
    static ImageOpenTransitionContext animationError(const QString &errorString);
};
}

#endif
