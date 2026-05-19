// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECONTAINEROPENPLAN_H
#define KIRIVIEW_IMAGECONTAINEROPENPLAN_H

#include "imagecandidatelistsource.h"
#include "imagecandidaterepository.h"

#include <QUrl>
#include <optional>
#include <vector>

namespace KiriView {
struct ImageContainerOpenPlan {
    std::optional<ImageCandidateListSource> source;
    ImageCandidateRepositoryError error = ImageCandidateRepositoryError::Generic;

    bool shouldLoadCandidates() const;
};

struct ImageContainerOpenResult {
    std::optional<QUrl> imageUrl;
    ImageCandidateRepositoryError error = ImageCandidateRepositoryError::Generic;

    bool openedImage() const;
};

ImageContainerOpenPlan imageContainerOpenPlanForCandidate(
    const ContainerNavigationCandidate &container);
ImageContainerOpenResult imageContainerOpenResultForCandidates(
    const std::vector<ImageNavigationCandidate> &candidates);
}

#endif
