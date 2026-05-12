// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADDOCUMENTCHANGE_H
#define KIRIVIEW_IMAGESPREADDOCUMENTCHANGE_H

#include "imagedocumenttypes.h"

namespace KiriView {
struct ImageSpreadDocumentChangePlan {
    bool finishTransition = false;
    bool refreshSecondaryPage = false;
    bool notifyRightToLeftReading = false;
};

ImageSpreadDocumentChangePlan imageSpreadDocumentChangePlan(
    ImageDocumentChange change, bool errorStringEmpty);
}

#endif
