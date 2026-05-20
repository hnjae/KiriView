// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_HEIFDECODER_H
#define KIRIVIEW_HEIFDECODER_H

#include "decodedimageresult.h"

#include <QByteArray>
#include <optional>

namespace KiriView {
std::optional<DecodedImageResult> decodeHeifStillImageData(const QByteArray &data);
std::optional<DecodedImageResult> decodeHeifSequenceImageData(const QByteArray &data);
std::optional<DecodedImageResult> decodeHeifImageData(const QByteArray &data);
}

#endif
