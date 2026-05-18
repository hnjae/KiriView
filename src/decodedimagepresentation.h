// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DECODEDIMAGEPRESENTATION_H
#define KIRIVIEW_DECODEDIMAGEPRESENTATION_H

#include "decodedimageresult.h"

#include <variant>

namespace KiriView {
struct DecodedStaticImagePresentation {
    StaticImagePayload staticImage;
    bool predecodeCacheable = false;
};

struct DecodedApngAnimationPresentation {
    QImage firstFrame;
    QByteArray data;
};

struct DecodedReaderAnimationPresentation {
    QImage firstFrame;
    QByteArray data;
    QByteArray format;
};

struct DecodedHeifSequenceAnimationPresentation {
    QImage firstFrame;
    QByteArray data;
};

struct UnpresentableDecodedImage {
};

using DecodedImagePresentation = std::variant<DecodedStaticImagePresentation,
    DecodedApngAnimationPresentation, DecodedReaderAnimationPresentation,
    DecodedHeifSequenceAnimationPresentation, UnpresentableDecodedImage>;

DecodedImagePresentation decodedImagePresentationForImage(DecodedImage image);
}

#endif
