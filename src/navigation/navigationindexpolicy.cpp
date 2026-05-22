// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigationindexpolicy.h"

#include "kiriview/src/policy/imagenavigationmodel.cxx.h"

namespace {
KiriView::RustNavigationDirection rustNavigationDirection(KiriView::NavigationDirection direction)
{
    switch (direction) {
    case KiriView::NavigationDirection::Previous:
        return KiriView::RustNavigationDirection::Previous;
    case KiriView::NavigationDirection::Next:
        return KiriView::RustNavigationDirection::Next;
    }

    return KiriView::RustNavigationDirection::Next;
}

KiriView::RustNavigationIndex rustNavigationIndex(std::optional<std::size_t> index)
{
    return KiriView::RustNavigationIndex { index.has_value(), index.value_or(0) };
}

std::optional<std::size_t> navigationIndexValue(KiriView::RustNavigationIndex index)
{
    if (!index.found) {
        return std::nullopt;
    }

    return index.index;
}
}

namespace KiriView {
std::optional<std::size_t> adjacentNavigationCandidateIndex(std::size_t candidateCount,
    std::optional<std::size_t> currentIndex, NavigationDirection direction)
{
    return navigationIndexValue(rustAdjacentNavigationCandidateIndex(
        candidateCount, rustNavigationIndex(currentIndex), rustNavigationDirection(direction)));
}
}
