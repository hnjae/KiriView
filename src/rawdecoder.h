// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_RAWDECODER_H
#define KIRIVIEW_RAWDECODER_H

#include "decodedimageresult.h"
#include "imagedecoderequest.h"

#include <QByteArray>
#include <optional>

namespace KiriView {
std::optional<DecodedImageResult> decodeRawImageData(
    const QByteArray &data, const ImageDecodeRequest &request);
}

#endif
