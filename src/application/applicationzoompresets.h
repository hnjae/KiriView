// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONZOOMPRESETS_H
#define KIRIVIEW_APPLICATIONZOOMPRESETS_H

#include "application/applicationtypes.h"

#include <KLazyLocalizedString>
#include <array>

namespace kiriview::ApplicationActions {
struct ZoomPresetDescriptor {
    ActionId actionId;
    const char *actionName;
    double zoomPercent = 100.0;
    KLazyLocalizedString actionText;
    KLazyLocalizedString menuText;
};

inline constexpr std::array<ZoomPresetDescriptor, 3> zoomPresetDescriptors {
    ZoomPresetDescriptor { ActionId::ViewZoom50PercentAction, "view_zoom_50_percent", 50.0,
        kli18nc("@action", "Zoom to 50%"), kli18nc("@action:inmenu", "Zoom to &50%") },
    ZoomPresetDescriptor { ActionId::ViewZoom100PercentAction, "view_zoom_100_percent", 100.0,
        kli18nc("@action", "Zoom to 100%"), kli18nc("@action:inmenu", "Zoom to &100%") },
    ZoomPresetDescriptor { ActionId::ViewZoom200PercentAction, "view_zoom_200_percent", 200.0,
        kli18nc("@action", "Zoom to 200%"), kli18nc("@action:inmenu", "Zoom to &200%") },
};

constexpr const ZoomPresetDescriptor *zoomPresetDescriptorForAction(ActionId actionId)
{
    for (const ZoomPresetDescriptor &descriptor : zoomPresetDescriptors) {
        if (descriptor.actionId == actionId) {
            return &descriptor;
        }
    }

    return nullptr;
}
}

#endif
