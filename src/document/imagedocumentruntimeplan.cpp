// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntimeplan.h"

namespace KiriView {
ImageDocumentRuntimePlan imageDocumentClearImagePlan()
{
    return {
        ClearArchiveSessionOperation {},
        ClearPredecodeOperation {},
        FinishSpreadTransitionOperation {},
        ClearSecondaryPageOperation {},
        CancelPageNavigationUpdateOperation {},
        ClearDisplayedImageLocationOperation {},
        ClearPresentationImageOperation {},
        ClearPageNavigationOperation {},
        NotifyRightToLeftReadingChangedOperation {},
    };
}

ImageDocumentRuntimePlan imageDocumentClearDeletedImagePlan()
{
    return {
        ClearArchiveSessionOperation {},
        CancelAllNavigationOperation {},
        CancelPredecodeOperation {},
        CancelOpenOperation {},
        FinishSpreadTransitionOperation {},
        ClearSecondaryPageOperation {},
        SetSourceUrlOperation { ImageNavigationTarget {} },
        SetErrorStringOperation { QString() },
        FinishEmptySourceLoadOperation {},
    };
}

ImageDocumentRuntimePlan imageDocumentShutdownPlan()
{
    return {
        CancelFileDeletionOperation {},
        StopPresentationAnimationOperation {},
        ShutdownSpreadOperation {},
        CancelPredecodeOperation {},
        CancelAllNavigationOperation {},
        CancelOpenOperation {},
        ClearArchiveSessionOperation {},
    };
}
}
