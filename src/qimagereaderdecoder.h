// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_QIMAGEREADERDECODER_H
#define KIRIVIEW_QIMAGEREADERDECODER_H

#include "decodedimageresult.h"
#include "imagedecoderequest.h"

#include <QByteArray>

namespace KiriView {
DecodedImageResult decodeQImageReaderImageData(
    const QByteArray &data, const ImageDecodeRequest &request);
}

#endif
