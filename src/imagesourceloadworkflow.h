// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESOURCELOADWORKFLOW_H
#define KIRIVIEW_IMAGESOURCELOADWORKFLOW_H

#include <vector>

namespace KiriView {
enum class ImageSourceLoadAction {
    CancelNavigationAndPredecode,
    FinishSpreadTransition,
    ResetRightToLeftReading,
    NotifyRightToLeftReading,
    ClearSecondaryPage,
    ClearLoadingContainerNavigationUrl,
    UpdateContainerNavigationUrl,
    SetLoadingContainerNavigationUrl,
    SetSourceUrl,
    BeginOpen,
};

struct ImageSourceLoadPlan {
    std::vector<ImageSourceLoadAction> actions;
};

struct ImageSourceLoadRequest {
    bool sourceUrlChanged = false;
    bool preserveTwoPageSpreadTransition = false;
    bool resetRightToLeftReading = false;
    bool rightToLeftReadingEnabled = false;
    bool containerNavigationUrlEmpty = false;
};

namespace ImageSourceLoadWorkflow {
    ImageSourceLoadPlan plan(const ImageSourceLoadRequest &request);
}
}

#endif
