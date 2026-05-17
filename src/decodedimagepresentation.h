// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DECODEDIMAGEPRESENTATION_H
#define KIRIVIEW_DECODEDIMAGEPRESENTATION_H

#include "decodedimageresult.h"

#include <variant>

namespace KiriView {
enum class DecodedImageAnimationKind {
    Apng,
    Reader,
    HeifSequence,
};

struct DecodedStaticImagePresentation {
    StaticImagePayload staticImage;
    bool predecodeCacheable = false;
};

struct DecodedAnimationImagePresentation {
    DecodedImageAnimationKind kind = DecodedImageAnimationKind::Reader;
    QImage firstFrame;
    QByteArray data;
    QByteArray format;
    int loopCount = 0;
    int firstFrameDelay = 0;
};

struct UnpresentableDecodedImage {
};

using DecodedImagePresentation = std::variant<DecodedStaticImagePresentation,
    DecodedAnimationImagePresentation, UnpresentableDecodedImage>;

DecodedImagePresentation decodedImagePresentationForImage(DecodedImage image);
}

#endif
