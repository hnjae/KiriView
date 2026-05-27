// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECONTAINEROPENPLAN_H
#define KIRIVIEW_IMAGECONTAINEROPENPLAN_H

#include "imagecandidatelistsource.h"
#include "imagenavigationtypes.h"

#include <QUrl>
#include <optional>
#include <vector>

namespace KiriView {
enum class ImageContainerOpenError {
    Generic,
    EmptyContainer,
    InvalidComicBookArchive,
};

struct ImageContainerOpenPlan {
    std::optional<ImageCandidateListSource> source;
    ImageContainerOpenError error = ImageContainerOpenError::Generic;

    bool shouldLoadCandidates() const;
};

struct ImageContainerOpenResult {
    std::optional<ImageNavigationTarget> target;
    ImageContainerOpenError error = ImageContainerOpenError::Generic;

    bool openedImage() const;
};

ImageContainerOpenPlan imageContainerOpenPlanForCandidate(
    const ContainerNavigationCandidate &container);
ImageContainerOpenResult imageContainerOpenResultForCandidates(
    const std::vector<ImageNavigationCandidate> &candidates);
}

#endif
