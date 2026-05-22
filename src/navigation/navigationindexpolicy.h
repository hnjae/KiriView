// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_NAVIGATIONINDEXPOLICY_H
#define KIRIVIEW_NAVIGATIONINDEXPOLICY_H

#include "imagenavigationtypes.h"

#include <cstddef>
#include <optional>

namespace KiriView {
std::optional<std::size_t> adjacentNavigationCandidateIndex(std::size_t candidateCount,
    std::optional<std::size_t> currentIndex, NavigationDirection direction);
}

#endif
