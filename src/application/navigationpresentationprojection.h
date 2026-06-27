// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_NAVIGATIONPRESENTATIONPROJECTION_H
#define KIRIVIEW_NAVIGATIONPRESENTATIONPROJECTION_H

#include "applicationtypes.h"

#include <array>

namespace kiriview::ApplicationActions {
struct NavigationPresentationSlot
{
    ActionId actionId = ActionId::ActionCount;
    ActionId iconActionId = ActionId::ActionCount;
};

struct NavigationPresentationProjection
{
    NavigationPresentationSlot leadingImageAction;
    NavigationPresentationSlot trailingImageAction;
    NavigationPresentationSlot leadingImageMenuAction;
    NavigationPresentationSlot trailingImageMenuAction;
    NavigationPresentationSlot firstImageMenuAction;
    NavigationPresentationSlot lastImageMenuAction;
    NavigationPresentationSlot leadingArchiveMenuAction;
    NavigationPresentationSlot trailingArchiveMenuAction;
    std::array<ActionId, 2> applicationMenuArchiveActionIds {
        ActionId::ActionCount,
        ActionId::ActionCount,
    };
};

NavigationPresentationProjection navigationPresentationProjection(bool rightToLeftReadingActive);
}

#endif
