// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOADTYPES_H
#define KIRIVIEW_IMAGELOADTYPES_H

#include "imagelocation.h"

#include <QtGlobal>

namespace KiriView {
struct ImageLoadSession {
    quint64 id = 0;
    ImageLoadRequest request;
    DisplayedImageLocation location;
};

enum class ImageLoadError {
    Generic,
    EmptyArchive,
};
}

#endif
