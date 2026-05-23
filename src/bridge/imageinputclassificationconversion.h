// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEINPUTCLASSIFICATIONCONVERSION_H
#define KIRIVIEW_IMAGEINPUTCLASSIFICATIONCONVERSION_H

#include "decoding/imageinputclassification.h"
#include "kiriview/src/policy/imageinputclassification.cxx.h"

namespace KiriView {
ImageInputClassification imageInputClassificationFromBridge(
    const RustImageInputClassification &classification);
}

#endif
