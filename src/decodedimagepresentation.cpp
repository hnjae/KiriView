// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedimagepresentation.h"

#include "predecodecache.h"

#include <utility>
#include <variant>

namespace {
KiriView::DecodedImagePresentation presentationForDecoded(KiriView::StaticDecodedImage &decoded)
{
    const bool predecodeCacheable = KiriView::PredecodeCache::canCacheImage(decoded.staticImage);
    return KiriView::DecodedStaticImagePresentation {
        std::move(decoded.staticImage),
        predecodeCacheable,
    };
}

KiriView::DecodedImagePresentation presentationForDecoded(KiriView::ApngAnimationImage &decoded)
{
    if (decoded.firstFrame.isNull()) {
        return KiriView::UnpresentableDecodedImage {};
    }

    return KiriView::DecodedApngAnimationPresentation {
        std::move(decoded.firstFrame),
        std::move(decoded.data),
    };
}

KiriView::DecodedImagePresentation presentationForDecoded(KiriView::ReaderAnimationImage &decoded)
{
    return KiriView::DecodedReaderAnimationPresentation {
        std::move(decoded.firstFrame),
        std::move(decoded.data),
        std::move(decoded.format),
    };
}

KiriView::DecodedImagePresentation presentationForDecoded(
    KiriView::HeifSequenceAnimationImage &decoded)
{
    return KiriView::DecodedHeifSequenceAnimationPresentation {
        std::move(decoded.firstFrame),
        std::move(decoded.data),
    };
}
}

namespace KiriView {
DecodedImagePresentation decodedImagePresentationForImage(DecodedImage image)
{
    return std::visit([](auto &decoded) { return presentationForDecoded(decoded); }, image);
}
}
