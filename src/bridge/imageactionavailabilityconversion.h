// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_BRIDGE_IMAGEACTIONAVAILABILITYCONVERSION_H
#define KIRIVIEW_BRIDGE_IMAGEACTIONAVAILABILITYCONVERSION_H

#include "application/imageactionavailabilitypolicy.h"
#include "kiriview/src/policy/imageactionavailability.cxx.h"

namespace kiriview::Bridge {
RustImageActionAvailabilityInput rustImageActionAvailabilityInput(
    ::ImageActionAvailabilityInput input);
RustImageActionAvailabilityProjection rustImageActionAvailabilityProjection(
    const ::ImageActionAvailabilityProjection& projection);
RustImageShortcutScope rustImageShortcutScope(ApplicationActions::ImageShortcutScope scope);
RustVideoShortcutAvailabilityInput rustVideoShortcutAvailabilityInput(
    ApplicationActions::VideoShortcutAvailabilityInput input);
::ImageActionAvailabilityProjection imageActionAvailabilityProjectionFromRust(
    const RustImageActionAvailabilityProjection& projection);
}

#endif
