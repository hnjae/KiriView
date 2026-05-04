// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEDECODER_H
#define KIRIVIEW_KIRIIMAGEDECODER_H

#include "decodedimageresult.h"
#include "imagedecoderequest.h"

#include <QByteArray>

namespace KiriView {
DecodedImageResult decodeImageData(const QByteArray &data);
DecodedImageResult decodeImageData(const QByteArray &data, const ImageDecodeRequest &request);
}

#endif
