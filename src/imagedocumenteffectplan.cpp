// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumenteffectplan.h"

#include <type_traits>
#include <variant>

namespace {
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
using KiriView::ImageDocumentRuntimeOperationKind;
using KiriView::ImageDocumentRuntimePlan;
using KiriView::LoadContainerImageOperation;
using KiriView::LoadPageNavigationUrlOperation;
using KiriView::LoadUrlOperation;
using KiriView::NotifyRightToLeftReadingChangedOperation;
using KiriView::OpenUrlEffect;
using KiriView::PageNavigationSelectedEffect;
using KiriView::PrepareFailedContainerEffect;
using KiriView::PrepareFailedContainerOperation;
using KiriView::ResetZoomEffect;
using KiriView::ResetZoomOperation;
using KiriView::ScheduleAdjacentImagePredecodeEffect;
using KiriView::ScheduleAdjacentImagePredecodeOperation;
using KiriView::SetErrorStringOperation;
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
ImageDocumentRuntimeOperationKind imageDocumentRuntimeOperationKind(
    const ImageDocumentRuntimeOperation &operation)
{
    return std::visit(
        [](const auto &payload) {
            using Operation = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<Operation, CancelFileDeletionOperation>) {
                return ImageDocumentRuntimeOperationKind::CancelFileDeletion;
            } else if constexpr (std::is_same_v<Operation, StopPresentationAnimationOperation>) {
                return ImageDocumentRuntimeOperationKind::StopPresentationAnimation;
            } else if constexpr (std::is_same_v<Operation, ShutdownSpreadOperation>) {
                return ImageDocumentRuntimeOperationKind::ShutdownSpread;
            } else if constexpr (std::is_same_v<Operation, ClearArchiveSessionOperation>) {
                return ImageDocumentRuntimeOperationKind::ClearArchiveSession;
            } else if constexpr (std::is_same_v<Operation, ClearPredecodeOperation>) {
                return ImageDocumentRuntimeOperationKind::ClearPredecode;
            } else if constexpr (std::is_same_v<Operation, CancelPredecodeOperation>) {
                return ImageDocumentRuntimeOperationKind::CancelPredecode;
            } else if constexpr (std::is_same_v<Operation,
                                     ScheduleAdjacentImagePredecodeOperation>) {
                return ImageDocumentRuntimeOperationKind::ScheduleAdjacentImagePredecode;
            } else if constexpr (std::is_same_v<Operation, FinishSpreadTransitionOperation>) {
                return ImageDocumentRuntimeOperationKind::FinishSpreadTransition;
            } else if constexpr (std::is_same_v<Operation, ClearSecondaryPageOperation>) {
                return ImageDocumentRuntimeOperationKind::ClearSecondaryPage;
            } else if constexpr (std::is_same_v<Operation,
                                     NotifyRightToLeftReadingChangedOperation>) {
                return ImageDocumentRuntimeOperationKind::NotifyRightToLeftReadingChanged;
            } else if constexpr (std::is_same_v<Operation, ResetZoomOperation>) {
                return ImageDocumentRuntimeOperationKind::ResetZoom;
            } else if constexpr (std::is_same_v<Operation, PrepareFailedContainerOperation>) {
                return ImageDocumentRuntimeOperationKind::PrepareFailedContainer;
            } else if constexpr (std::is_same_v<Operation, CancelPageNavigationUpdateOperation>) {
                return ImageDocumentRuntimeOperationKind::CancelPageNavigationUpdate;
            } else if constexpr (std::is_same_v<Operation, CancelNavigationOperation>) {
                return ImageDocumentRuntimeOperationKind::CancelNavigation;
            } else if constexpr (std::is_same_v<Operation, CancelContainerNavigationOperation>) {
                return ImageDocumentRuntimeOperationKind::CancelContainerNavigation;
            } else if constexpr (std::is_same_v<Operation, CancelAllNavigationOperation>) {
                return ImageDocumentRuntimeOperationKind::CancelAllNavigation;
            } else if constexpr (std::is_same_v<Operation, ClearPageNavigationOperation>) {
                return ImageDocumentRuntimeOperationKind::ClearPageNavigation;
            } else if constexpr (std::is_same_v<Operation, UpdatePageNavigationOperation>) {
                return ImageDocumentRuntimeOperationKind::UpdatePageNavigation;
            } else if constexpr (std::is_same_v<Operation, LoadUrlOperation>) {
                return ImageDocumentRuntimeOperationKind::LoadUrl;
            } else if constexpr (std::is_same_v<Operation, LoadContainerImageOperation>) {
                return ImageDocumentRuntimeOperationKind::LoadContainerImage;
            } else if constexpr (std::is_same_v<Operation,
                                     FinishEmptyContainerNavigationOperation>) {
                return ImageDocumentRuntimeOperationKind::FinishEmptyContainerNavigation;
            } else if constexpr (std::is_same_v<Operation,
                                     FinishContainerNavigationLoadWithErrorOperation>) {
                return ImageDocumentRuntimeOperationKind::FinishContainerNavigationLoadWithError;
            } else if constexpr (std::is_same_v<Operation, LoadPageNavigationUrlOperation>) {
                return ImageDocumentRuntimeOperationKind::LoadPageNavigationUrl;
            } else if constexpr (std::is_same_v<Operation, CancelOpenOperation>) {
                return ImageDocumentRuntimeOperationKind::CancelOpen;
            } else if constexpr (std::is_same_v<Operation, ClearDisplayedImageLocationOperation>) {
                return ImageDocumentRuntimeOperationKind::ClearDisplayedImageLocation;
            } else if constexpr (std::is_same_v<Operation, ClearPresentationImageOperation>) {
                return ImageDocumentRuntimeOperationKind::ClearPresentationImage;
            } else if constexpr (std::is_same_v<Operation, SetSourceUrlOperation>) {
                return ImageDocumentRuntimeOperationKind::SetSourceUrl;
            } else if constexpr (std::is_same_v<Operation, SetErrorStringOperation>) {
                return ImageDocumentRuntimeOperationKind::SetErrorString;
            } else {
                return ImageDocumentRuntimeOperationKind::FinishEmptySourceLoad;
            }
        },
        operation);
}

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
