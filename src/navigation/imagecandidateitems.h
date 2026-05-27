// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECANDIDATEITEMS_H
#define KIRIVIEW_IMAGECANDIDATEITEMS_H

#include "imagenavigationtypes.h"
#include "medianavigationmodel.h"

#include <KFileItem>
#include <vector>

namespace KiriView {
std::vector<ImageNavigationCandidate> imageNavigationCandidates(const KFileItemList &items);
std::vector<MediaNavigationCandidate> mediaNavigationCandidates(const KFileItemList &items);
std::vector<ContainerNavigationCandidate> containerNavigationCandidates(const KFileItemList &items);
}

#endif
