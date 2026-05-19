// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENTRANSITIONAPPLIER_H
#define KIRIVIEW_IMAGEOPENTRANSITIONAPPLIER_H

#include "imagedocumenteffects.h"
#include "imagedocumenttypes.h"
#include "imageloadtypes.h"
#include "imagelocation.h"

#include <QString>
#include <QUrl>
#include <optional>
#include <vector>

namespace KiriView {
class ImageDocumentState;

enum class ImageOpenBoolTarget {
    Unchanged = 0,
    False = 1,
    True = 2,
};

enum class ImageOpenStatusTarget {
    Unchanged = 0,
    Null = 1,
    Loading = 2,
    Ready = 3,
    Error = 4,
};

enum class ImageOpenErrorStringTarget {
    Unchanged = 0,
    Clear = 1,
    Provided = 2,
};

enum class ImageOpenUrlTarget {
    Unchanged = 0,
    Empty = 1,
    SessionImage = 2,
    SessionContainerNavigation = 3,
    DerivedContainerNavigation = 4,
    Container = 5,
    Displayed = 6,
};

enum class ImageOpenDisplayedLocationTarget {
    Unchanged = 0,
    Session = 1,
};

enum class ImageOpenEffect {
    ClearImage = 0,
    ResetZoom = 1,
    UpdatePageNavigation = 2,
    ScheduleAdjacentImagePredecode = 3,
    PrepareFailedContainer = 4,
};

struct ImageOpenStateDelta {
    ImageOpenUrlTarget sourceUrl = ImageOpenUrlTarget::Unchanged;
    ImageOpenDisplayedLocationTarget displayedLocation
        = ImageOpenDisplayedLocationTarget::Unchanged;
    ImageOpenUrlTarget containerNavigationUrl = ImageOpenUrlTarget::Unchanged;
    ImageOpenBoolTarget loading = ImageOpenBoolTarget::Unchanged;
    ImageOpenStatusTarget status = ImageOpenStatusTarget::Unchanged;
    ImageOpenErrorStringTarget errorString = ImageOpenErrorStringTarget::Unchanged;
    bool clearLoadingContainerNavigationUrl = false;
};

struct ImageOpenTransition {
    ImageOpenStateDelta stateDelta;
    std::vector<ImageOpenEffect> effects;
};

struct ImageOpenResolvedStateDelta {
    std::optional<QUrl> sourceUrl;
    std::optional<DisplayedImageLocation> displayedLocation;
    std::optional<QUrl> containerNavigationUrl;
    std::optional<bool> loading;
    std::optional<ImageDocumentStatus> status;
    std::optional<QString> errorString;
    bool clearLoadingContainerNavigationUrl = false;
};

struct ImageOpenApplicationPlan {
    ImageOpenResolvedStateDelta stateDelta;
    ImageDocumentEffects effects;
};

struct ImageOpenTransitionContext {
    const ImageLoadSession *session = nullptr;
    std::optional<QUrl> containerUrl;
    std::optional<QUrl> displayedUrl;
    std::optional<QString> errorString;

    static ImageOpenTransitionContext successfulImageLoad(const ImageLoadSession &session);
    static ImageOpenTransitionContext sourceLoadError(
        const ImageLoadSession &session, const QUrl &displayedUrl, const QString &errorString);
    static ImageOpenTransitionContext containerNavigationError(
        const QUrl &containerUrl, const QString &errorString);
    static ImageOpenTransitionContext animationError(const QString &errorString);
};

ImageOpenApplicationPlan imageOpenApplicationPlan(
    const ImageOpenTransition &transition, const ImageOpenTransitionContext &context = {});
ImageDocumentEffects applyImageOpenTransition(ImageDocumentState &state,
    const ImageOpenTransition &transition, const ImageOpenTransitionContext &context = {});
}

#endif
