// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumenteffectplan.h"

#include <utility>
#include <variant>

namespace {
using KiriView::ClearDeletedImageEffect;
using KiriView::ClearImageEffect;
using KiriView::ContainerImageSelectedEffect;
using KiriView::ContainerNavigationFailedEffect;
using KiriView::EmptyContainerSelectedEffect;
using KiriView::ImageDocumentRuntimeOperation;
using KiriView::ImageDocumentRuntimeOperationKind;
using KiriView::ImageDocumentRuntimePlan;
using KiriView::OpenUrlEffect;
using KiriView::PageNavigationSelectedEffect;
using KiriView::PrepareFailedContainerEffect;
using KiriView::ResetZoomEffect;
using KiriView::ScheduleAdjacentImagePredecodeEffect;
using KiriView::UpdatePageNavigationEffect;

ImageDocumentRuntimePlan clearImagePlan()
{
    return {
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::ClearArchiveSession),
        ImageDocumentRuntimeOperation::simple(ImageDocumentRuntimeOperationKind::ClearPredecode),
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::FinishSpreadTransition),
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::ClearSecondaryPage),
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::CancelPageNavigationUpdate),
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::ClearDisplayedImageLocation),
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::ClearPresentationImage),
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::ClearPageNavigation),
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::NotifyRightToLeftReadingChanged),
    };
}

ImageDocumentRuntimePlan clearDeletedImagePlan()
{
    return {
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::ClearArchiveSession),
        ImageDocumentRuntimeOperation::simple(ImageDocumentRuntimeOperationKind::CancelNavigation),
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::CancelContainerNavigation),
        ImageDocumentRuntimeOperation::simple(ImageDocumentRuntimeOperationKind::CancelPredecode),
        ImageDocumentRuntimeOperation::simple(ImageDocumentRuntimeOperationKind::CancelOpen),
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::FinishSpreadTransition),
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::ClearSecondaryPage),
        ImageDocumentRuntimeOperation::setSourceUrl(QUrl()),
        ImageDocumentRuntimeOperation::setErrorString(QString()),
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::FinishEmptySourceLoad),
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
        return { ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::ResetZoom) };
    }

    ImageDocumentRuntimePlan operator()(const UpdatePageNavigationEffect &) const
    {
        return { ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::UpdatePageNavigation) };
    }

    ImageDocumentRuntimePlan operator()(const ScheduleAdjacentImagePredecodeEffect &) const
    {
        return { ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::ScheduleAdjacentImagePredecode) };
    }

    ImageDocumentRuntimePlan operator()(const OpenUrlEffect &effect) const
    {
        return { ImageDocumentRuntimeOperation::loadUrl(effect.url) };
    }

    ImageDocumentRuntimePlan operator()(const ContainerImageSelectedEffect &effect) const
    {
        return { ImageDocumentRuntimeOperation::loadContainerImage(
            effect.imageUrl, effect.containerUrl) };
    }

    ImageDocumentRuntimePlan operator()(const EmptyContainerSelectedEffect &effect) const
    {
        return { ImageDocumentRuntimeOperation::finishEmptyContainerNavigation(
            effect.containerUrl) };
    }

    ImageDocumentRuntimePlan operator()(const ContainerNavigationFailedEffect &effect) const
    {
        return { ImageDocumentRuntimeOperation::finishContainerNavigationLoadWithError(
            effect.containerUrl, effect.errorString) };
    }

    ImageDocumentRuntimePlan operator()(const PageNavigationSelectedEffect &effect) const
    {
        return { ImageDocumentRuntimeOperation::loadPageNavigationUrl(
            effect.url, effect.preserveTwoPageSpreadTransition) };
    }

    ImageDocumentRuntimePlan operator()(const PrepareFailedContainerEffect &effect) const
    {
        return { ImageDocumentRuntimeOperation::prepareFailedContainer(effect.containerUrl) };
    }
};
}

namespace KiriView {
ImageDocumentRuntimeOperation ImageDocumentRuntimeOperation::simple(
    ImageDocumentRuntimeOperationKind kind)
{
    ImageDocumentRuntimeOperation operation;
    operation.kind = kind;
    return operation;
}

ImageDocumentRuntimeOperation ImageDocumentRuntimeOperation::loadUrl(QUrl url)
{
    ImageDocumentRuntimeOperation operation = simple(ImageDocumentRuntimeOperationKind::LoadUrl);
    operation.url = std::move(url);
    return operation;
}

ImageDocumentRuntimeOperation ImageDocumentRuntimeOperation::loadContainerImage(
    QUrl imageUrl, QUrl containerUrl)
{
    ImageDocumentRuntimeOperation operation
        = simple(ImageDocumentRuntimeOperationKind::LoadContainerImage);
    operation.url = std::move(imageUrl);
    operation.secondaryUrl = std::move(containerUrl);
    return operation;
}

ImageDocumentRuntimeOperation ImageDocumentRuntimeOperation::finishEmptyContainerNavigation(
    QUrl containerUrl)
{
    ImageDocumentRuntimeOperation operation
        = simple(ImageDocumentRuntimeOperationKind::FinishEmptyContainerNavigation);
    operation.url = std::move(containerUrl);
    return operation;
}

ImageDocumentRuntimeOperation ImageDocumentRuntimeOperation::finishContainerNavigationLoadWithError(
    QUrl containerUrl, QString errorString)
{
    ImageDocumentRuntimeOperation operation
        = simple(ImageDocumentRuntimeOperationKind::FinishContainerNavigationLoadWithError);
    operation.url = std::move(containerUrl);
    operation.errorString = std::move(errorString);
    return operation;
}

ImageDocumentRuntimeOperation ImageDocumentRuntimeOperation::loadPageNavigationUrl(
    QUrl url, bool preserveTwoPageSpreadTransition)
{
    ImageDocumentRuntimeOperation operation
        = simple(ImageDocumentRuntimeOperationKind::LoadPageNavigationUrl);
    operation.url = std::move(url);
    operation.preserveTwoPageSpreadTransition = preserveTwoPageSpreadTransition;
    return operation;
}

ImageDocumentRuntimeOperation ImageDocumentRuntimeOperation::prepareFailedContainer(
    QUrl containerUrl)
{
    ImageDocumentRuntimeOperation operation
        = simple(ImageDocumentRuntimeOperationKind::PrepareFailedContainer);
    operation.url = std::move(containerUrl);
    return operation;
}

ImageDocumentRuntimeOperation ImageDocumentRuntimeOperation::setSourceUrl(QUrl url)
{
    ImageDocumentRuntimeOperation operation
        = simple(ImageDocumentRuntimeOperationKind::SetSourceUrl);
    operation.url = std::move(url);
    return operation;
}

ImageDocumentRuntimeOperation ImageDocumentRuntimeOperation::setErrorString(QString errorString)
{
    ImageDocumentRuntimeOperation operation
        = simple(ImageDocumentRuntimeOperationKind::SetErrorString);
    operation.errorString = std::move(errorString);
    return operation;
}

bool ImageDocumentRuntimeOperation::operator==(const ImageDocumentRuntimeOperation &other) const
{
    return kind == other.kind && url == other.url && secondaryUrl == other.secondaryUrl
        && errorString == other.errorString
        && preserveTwoPageSpreadTransition == other.preserveTwoPageSpreadTransition;
}

ImageDocumentRuntimePlan imageDocumentEffectPlan(const ImageDocumentEffect &effect)
{
    return std::visit(ImageDocumentEffectPlanVisitor {}, effect.payload);
}

ImageDocumentRuntimePlan imageDocumentShutdownPlan()
{
    return {
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::CancelFileDeletion),
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::StopPresentationAnimation),
        ImageDocumentRuntimeOperation::simple(ImageDocumentRuntimeOperationKind::ShutdownSpread),
        ImageDocumentRuntimeOperation::simple(ImageDocumentRuntimeOperationKind::CancelPredecode),
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::CancelPageNavigationUpdate),
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::CancelContainerNavigation),
        ImageDocumentRuntimeOperation::simple(ImageDocumentRuntimeOperationKind::CancelNavigation),
        ImageDocumentRuntimeOperation::simple(ImageDocumentRuntimeOperationKind::CancelOpen),
        ImageDocumentRuntimeOperation::simple(
            ImageDocumentRuntimeOperationKind::ClearArchiveSession),
    };
}
}
