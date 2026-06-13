// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_STATICIMAGEDECODE_H
#define KIRIVIEW_STATICIMAGEDECODE_H

#include "decodedimageresult.h"

#include <QString>
#include <memory>

namespace kiriview {
class ImageDecodeRequest;

DecodedImageResult staticDecodedImageResult(std::shared_ptr<ImageTileSource> source,
    const ImageDecodeRequest &request, QString *errorString);
}

#endif
