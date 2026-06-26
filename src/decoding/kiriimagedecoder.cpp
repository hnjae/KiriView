// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagedecoder.h"

#include "imagedecodepipeline.h"

namespace kiriview {
DecodedImageResult decodeImageData(const QByteArray& data)
{
    return decodeImageData(data, ImageDecodeRequest {});
}

DecodedImageResult decodeImageData(const QByteArray& data, const ImageDecodeRequest& request)
{
    return decodeImageDataWithDefaultRouter(data, request);
}

}
