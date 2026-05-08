// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTCONTROLLER_H

#include "filedeletion.h"
#include "imageasyncdependencies.h"
#include "imagecandidaterepository.h"
#include "imagedocumenteffects.h"
#include "imagedocumentstate.h"
#include "imageloadtypes.h"
#include "imagesurface.h"
#include "imagezoomstate.h"
#include "predecodedimage.h"

#include <QImage>
#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <memory>
#include <optional>

namespace KiriView {
class ImageDocumentNavigationController;
class ImageOpenController;
class ImagePresentationController;
class ImagePredecodeCoordinator;

class ImageDocumentController final : public QObject
{
public:
    using RenderContextProvider = std::function<ImageDocumentRenderContext()>;
    using ChangeCallback = std::function<void(ImageDocumentChange)>;
    using FileDeletionFailedCallback = std::function<void(const QString &)>;

    ImageDocumentController(QObject *parent, RenderContextProvider renderContextProvider,
        ChangeCallback changeCallback);
    ImageDocumentController(QObject *parent, RenderContextProvider renderContextProvider,
        ChangeCallback changeCallback, ImageAsyncDependencies dependencies,
        FileDeletionFailedCallback fileDeletionFailedCallback = {});
    ~ImageDocumentController() override;

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl &sourceUrl);
    ImageDocumentStatus status() const;
    bool loading() const;
    QString errorString() const;
    QString windowTitleFileName() const;
    QUrl displayedUrl() const;
    QSize imageSize() const;
    QSizeF viewportSize() const;
    void setViewportSize(const QSizeF &viewportSize);
    QRectF visibleItemRect() const;
    void setVisibleItemRect(const QRectF &visibleItemRect);
    QSizeF displaySize() const;
    qreal zoomPercent() const;
    void setZoomPercent(qreal zoomPercent);
    ImageZoomMode zoomMode() const;
    int currentPageNumber() const;
    int imageCount() const;
    bool containerNavigationAvailable() const;
    bool fileDeletionInProgress() const;
    std::shared_ptr<DisplayedImageSurface> imageSurface() const;
    const QImage &image() const;
    quint64 imageRevision() const;

    void openPreviousImage();
    void openNextImage();
    void openImageAtPage(int pageNumber);
    void openPreviousContainer();
    void openNextContainer();
    void deleteDisplayedFile(FileDeletionMode mode);
    void resetZoom();
    void setFitMode(ImageZoomMode zoomMode);
    void updateRenderContext();

private:
    struct DeletionFallbackPlan;

    void dispatchEffect(ImageDocumentEffect effect);
    void dispatchEffectPayload(const ClearImageEffect &);
    void dispatchEffectPayload(const ResetZoomEffect &);
    void dispatchEffectPayload(const UpdatePageNavigationEffect &);
    void dispatchEffectPayload(const ScheduleAdjacentImagePredecodeEffect &);
    void dispatchEffectPayload(const OpenUrlEffect &payload);
    void dispatchEffectPayload(const ContainerImageSelectedEffect &payload);
    void dispatchEffectPayload(const EmptyContainerSelectedEffect &payload);
    void dispatchEffectPayload(const ContainerNavigationFailedEffect &payload);
    void dispatchEffectPayload(const PrepareFailedContainerEffect &payload);
    void dispatchEffectPayload(const AnimationFailedEffect &payload);
    void dispatchEffects(const ImageDocumentEffects &effects);
    void setSourceUrlForLoad(const QUrl &sourceUrl, const QUrl &containerNavigationUrl);
    void clearAfterSuccessfulFileDeletion();
    void finishFileDeletion(const DeletionFallbackPlan &fallbackPlan, FileDeletionResult result,
        const QString &errorString);
    void openDeletionFallback(const DeletionFallbackPlan &fallbackPlan);
    void openImageDeletionFallback(const DeletionFallbackPlan &fallbackPlan);
    void openComicBookDeletionFallback(const DeletionFallbackPlan &fallbackPlan);
    void openComicBookDeletionFallbackCandidate(
        const std::optional<ContainerNavigationCandidate> &candidate,
        const std::optional<ContainerNavigationCandidate> &fallbackCandidate);
    void setFileDeletionInProgress(bool inProgress);
    void cancelFileDeletion();
    void cancelFileDeletionFallback();
    void reportFileDeletionFailure(const QString &errorString);
    void scheduleAdjacentImagePredecode();
    void cancelPredecode();
    std::optional<PredecodedImage> takePredecodedImage(const QUrl &url) const;
    void finishWithAnimationError(const QString &errorString);
    void notify(ImageDocumentChange change);
    void clearImage();

    ChangeCallback m_changeCallback;
    FileDeletionFailedCallback m_fileDeletionFailedCallback;
    ImageDocumentState m_state;
    ImageCandidateRepository m_deletionCandidateRepository;
    FileOperationProvider m_fileOperationProvider;
    std::unique_ptr<ImagePresentationController> m_presentationController;
    std::unique_ptr<ImageOpenController> m_openController;
    std::unique_ptr<ImageDocumentNavigationController> m_navigationController;
    std::unique_ptr<ImagePredecodeCoordinator> m_predecodeCoordinator;
    ImageIoJob m_fileDeletionJob;
    ImageIoJob m_fileDeletionFallbackJob;
    bool m_fileDeletionInProgress = false;
};
}

#endif
