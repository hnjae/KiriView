// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODEDIMAGE_H
#define KIRIVIEW_PREDECODEDIMAGE_H

#include "location/imagelocation.h"
#include "rendering/staticimage.h"

#include <optional>

namespace KiriView {
struct PredecodedImage {
    StaticImagePayload staticImage;
    DisplayedImageLocation location;
};

struct DisplayedPredecodeImage {
    DisplayedImageLocation location;
    bool cacheable = false;
    std::optional<StaticImagePayload> staticImage;

    bool hasLocation() const { return !location.isEmpty(); }

    bool hasStaticImage() const { return staticImage.has_value() && staticImage->isValid(); }

    bool isCacheable() const { return cacheable && hasStaticImage(); }
};
}

#endif
