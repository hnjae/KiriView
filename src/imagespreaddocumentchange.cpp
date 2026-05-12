// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagespreaddocumentchange.h"

namespace KiriView {
ImageSpreadDocumentChangePlan imageSpreadDocumentChangePlan(
    ImageDocumentChange change, bool errorStringEmpty)
{
    return ImageSpreadDocumentChangePlan {
        change == ImageDocumentChange::ErrorString && !errorStringEmpty,
        change == ImageDocumentChange::PageNavigation,
        change == ImageDocumentChange::PageNavigation,
    };
}
}
