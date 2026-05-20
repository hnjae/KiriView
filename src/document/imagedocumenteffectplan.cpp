// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumenteffectplan.h"

#include <variant>

namespace {
using KiriView::BeginOpenOperation;
using KiriView::CancelAllNavigationOperation;
using KiriView::CancelContainerNavigationOperation;
using KiriView::CancelFileDeletionOperation;
using KiriView::CancelNavigationOperation;
using KiriView::CancelOpenOperation;
using KiriView::CancelPageNavigationUpdateOperation;
using KiriView::CancelPredecodeOperation;
using KiriView::ClearArchiveSessionOperation;
using KiriView::ClearDeletedImageEffect;
using KiriView::ClearDisplayedImageLocationOperation;
using KiriView::ClearImageEffect;
using KiriView::ClearLoadingContainerNavigationUrlOperation;
using KiriView::ClearPageNavigationOperation;
using KiriView::ClearPredecodeOperation;
using KiriView::ClearPresentationImageOperation;
using KiriView::ClearSecondaryPageOperation;
using KiriView::ContainerImageSelectedEffect;
using KiriView::ContainerNavigationFailedEffect;
using KiriView::EmptyContainerSelectedEffect;
using KiriView::FinishContainerNavigationLoadWithErrorOperation;
using KiriView::FinishEmptyContainerNavigationOperation;
using KiriView::FinishEmptySourceLoadOperation;
using KiriView::FinishSpreadTransitionOperation;
using KiriView::ImageDocumentRuntimePlan;
using KiriView::LoadContainerImageOperation;
using KiriView::LoadPageNavigationUrlOperation;
using KiriView::LoadUrlOperation;
using KiriView::NotifyRightToLeftReadingChangedOperation;
using KiriView::OpenUrlEffect;
using KiriView::PageNavigationSelectedEffect;
using KiriView::PrepareFailedContainerEffect;
using KiriView::PrepareFailedContainerOperation;
using KiriView::PrepareSourceLoadOperation;
using KiriView::ResetRightToLeftReadingOperation;
using KiriView::ResetZoomEffect;
using KiriView::ResetZoomOperation;
using KiriView::ScheduleAdjacentImagePredecodeEffect;
using KiriView::ScheduleAdjacentImagePredecodeOperation;
using KiriView::SetContainerNavigationUrlOperation;
using KiriView::SetErrorStringOperation;
using KiriView::SetLoadingContainerNavigationUrlOperation;
using KiriView::SetSourceUrlOperation;
using KiriView::ShutdownSpreadOperation;
using KiriView::StopPresentationAnimationOperation;
using KiriView::UpdatePageNavigationEffect;
using KiriView::UpdatePageNavigationOperation;

ImageDocumentRuntimePlan clearImagePlan()
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

ImageDocumentRuntimePlan clearDeletedImagePlan()
{
    return {
        ClearArchiveSessionOperation {},
        CancelAllNavigationOperation {},
        CancelPredecodeOperation {},
        CancelOpenOperation {},
        FinishSpreadTransitionOperation {},
        ClearSecondaryPageOperation {},
        SetSourceUrlOperation { QUrl() },
        SetErrorStringOperation { QString() },
        FinishEmptySourceLoadOperation {},
    };
}

struct ImageDocumentEffectPlanVisitor {
    ImageDocumentRuntimePlan operator()(const ClearImageEffect &) const { return clearImagePlan(); }

    ImageDocumentRuntimePlan operator()(const ClearDeletedImageEffect &) const
    {
        return clearDeletedImagePlan();
    }

    ImageDocumentRuntimePlan operator()(const ResetZoomEffect &) const
    {
        return { ResetZoomOperation {} };
    }

    ImageDocumentRuntimePlan operator()(const UpdatePageNavigationEffect &) const
    {
        return { UpdatePageNavigationOperation {} };
    }

    ImageDocumentRuntimePlan operator()(const ScheduleAdjacentImagePredecodeEffect &) const
    {
        return { ScheduleAdjacentImagePredecodeOperation {} };
    }

    ImageDocumentRuntimePlan operator()(const OpenUrlEffect &effect) const
    {
        return { LoadUrlOperation { effect.url } };
    }

    ImageDocumentRuntimePlan operator()(const ContainerImageSelectedEffect &effect) const
    {
        return { LoadContainerImageOperation { effect.imageUrl, effect.containerUrl } };
    }

    ImageDocumentRuntimePlan operator()(const EmptyContainerSelectedEffect &effect) const
    {
        return { FinishEmptyContainerNavigationOperation { effect.containerUrl } };
    }

    ImageDocumentRuntimePlan operator()(const ContainerNavigationFailedEffect &effect) const
    {
        return { FinishContainerNavigationLoadWithErrorOperation {
            effect.containerUrl,
            effect.errorString,
        } };
    }

    ImageDocumentRuntimePlan operator()(const PageNavigationSelectedEffect &effect) const
    {
        return { LoadPageNavigationUrlOperation {
            effect.url,
            effect.preserveTwoPageSpreadTransition,
        } };
    }

    ImageDocumentRuntimePlan operator()(const PrepareFailedContainerEffect &effect) const
    {
        return { PrepareFailedContainerOperation { effect.containerUrl } };
    }
};
}

namespace KiriView {
ImageDocumentRuntimePlan imageDocumentEffectPlan(const ImageDocumentEffect &effect)
{
    return std::visit(ImageDocumentEffectPlanVisitor {}, effect.payload);
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
