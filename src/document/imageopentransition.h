// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENTRANSITION_H
#define KIRIVIEW_IMAGEOPENTRANSITION_H

#include <vector>

namespace KiriView {
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

enum class ImageOpenSourceKindTarget {
    Unchanged = 0,
    Session = 1,
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
    ClearLoadingPresentation = 5,
};

struct ImageOpenStateDelta {
    ImageOpenUrlTarget sourceUrl = ImageOpenUrlTarget::Unchanged;
    ImageOpenSourceKindTarget sourceKind = ImageOpenSourceKindTarget::Unchanged;
    ImageOpenDisplayedLocationTarget displayedLocation
        = ImageOpenDisplayedLocationTarget::Unchanged;
    ImageOpenUrlTarget containerNavigationUrl = ImageOpenUrlTarget::Unchanged;
    ImageOpenBoolTarget loading = ImageOpenBoolTarget::Unchanged;
    ImageOpenStatusTarget status = ImageOpenStatusTarget::Unchanged;
    ImageOpenErrorStringTarget errorString = ImageOpenErrorStringTarget::Unchanged;
    ImageOpenBoolTarget unsupportedOpenedCollectionVideo = ImageOpenBoolTarget::Unchanged;
    bool clearEmbeddedMetadata = false;
    bool clearLoadingContainerNavigationUrl = false;
};

struct ImageOpenTransition {
    ImageOpenStateDelta stateDelta;
    std::vector<ImageOpenEffect> effects;
};
}

#endif
