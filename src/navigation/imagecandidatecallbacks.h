// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECANDIDATECALLBACKS_H
#define KIRIVIEW_IMAGECANDIDATECALLBACKS_H

#include "imagenavigationtypes.h"

#include <functional>
#include <vector>

namespace KiriView {
using ImageCandidatesCallback = std::function<void(std::vector<ImageNavigationCandidate>)>;
using ContainerCandidatesCallback = std::function<void(std::vector<ContainerNavigationCandidate>)>;
}

#endif
