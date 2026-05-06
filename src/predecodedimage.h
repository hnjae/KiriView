// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODEDIMAGE_H
#define KIRIVIEW_PREDECODEDIMAGE_H

#include "imagelocation.h"
#include "staticimage.h"

namespace KiriView {
struct PredecodedImage {
    StaticImagePayload staticImage;
    DisplayedImageLocation location;
};
}

#endif
