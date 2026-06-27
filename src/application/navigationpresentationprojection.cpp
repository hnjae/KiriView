// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigationpresentationprojection.h"

namespace {
namespace Actions = kiriview::ApplicationActions;
using ActionId = kiriview::ApplicationActions::ActionId;

Actions::NavigationPresentationSlot slot(ActionId actionId, ActionId iconActionId)
{
    return Actions::NavigationPresentationSlot { actionId, iconActionId };
}
}

namespace kiriview::ApplicationActions {
NavigationPresentationProjection navigationPresentationProjection(bool rightToLeftReadingActive)
{
    if (rightToLeftReadingActive) {
        return NavigationPresentationProjection {
            slot(ActionId::GoNextImageAction, ActionId::GoPreviousImageAction),
            slot(ActionId::GoPreviousImageAction, ActionId::GoNextImageAction),
            slot(ActionId::GoNextImageAction, ActionId::GoPreviousImageAction),
            slot(ActionId::GoPreviousImageAction, ActionId::GoNextImageAction),
            slot(ActionId::GoFirstImageAction, ActionId::GoLastImageAction),
            slot(ActionId::GoLastImageAction, ActionId::GoFirstImageAction),
            slot(ActionId::GoNextArchiveAction, ActionId::GoPreviousArchiveAction),
            slot(ActionId::GoPreviousArchiveAction, ActionId::GoNextArchiveAction),
            {
                ActionId::GoNextArchiveAction,
                ActionId::GoPreviousArchiveAction,
            },
        };
    }

    return NavigationPresentationProjection {
        slot(ActionId::GoPreviousImageAction, ActionId::GoPreviousImageAction),
        slot(ActionId::GoNextImageAction, ActionId::GoNextImageAction),
        slot(ActionId::GoPreviousImageAction, ActionId::GoPreviousImageAction),
        slot(ActionId::GoNextImageAction, ActionId::GoNextImageAction),
        slot(ActionId::GoFirstImageAction, ActionId::GoFirstImageAction),
        slot(ActionId::GoLastImageAction, ActionId::GoLastImageAction),
        slot(ActionId::GoPreviousArchiveAction, ActionId::GoPreviousArchiveAction),
        slot(ActionId::GoNextArchiveAction, ActionId::GoNextArchiveAction),
        {
            ActionId::GoPreviousArchiveAction,
            ActionId::GoNextArchiveAction,
        },
    };
}
}
