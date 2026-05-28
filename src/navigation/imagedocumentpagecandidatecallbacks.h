// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATECALLBACKS_H
#define KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATECALLBACKS_H

#include "imagedocumentpagenavigationtypes.h"

#include <functional>
#include <vector>

namespace KiriView {
using ImageDocumentPageCandidatesCallback
    = std::function<void(std::vector<ImageDocumentPageCandidate>)>;
using ContainerCandidatesCallback = std::function<void(std::vector<ContainerNavigationCandidate>)>;
}

#endif
