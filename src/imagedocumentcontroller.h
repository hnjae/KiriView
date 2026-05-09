// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTCONTROLLER_H

#include "filedeletion.h"
#include "imageasyncdependencies.h"
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
#include <map>
#include <memory>
#include <optional>

namespace KiriView {
class ImageDeletionController;
class ImageDocumentNavigationController;
class ImageLoader;
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
    QSize primaryImageSize() const;
    QSize secondaryImageSize() const;
    QSizeF viewportSize() const;
    void setViewportSize(const QSizeF &viewportSize);
    QRectF visibleItemRect() const;
    void setVisibleItemRect(const QRectF &visibleItemRect);
    QSizeF displaySize() const;
    QSizeF primaryDisplaySize() const;
    QSizeF secondaryDisplaySize() const;
    qreal zoomPercent() const;
    void setZoomPercent(qreal zoomPercent);
    ImageZoomMode zoomMode() const;
    qreal maximumManualZoomPercent() const;
    qreal clampedManualZoomPercent(qreal zoomPercent) const;
    int currentPageNumber() const;
    int currentLastPageNumber() const;
    int imageCount() const;
    bool containerNavigationAvailable() const;
    bool fileDeletionInProgress() const;
    bool twoPageModeEnabled() const;
    void setTwoPageModeEnabled(bool enabled);
    bool twoPageModeAvailable() const;
    bool secondaryPageVisible() const;
    std::shared_ptr<DisplayedImageSurface> imageSurface(
        DisplayedPageRole role = DisplayedPageRole::Primary) const;
    const QImage &image() const;
    quint64 imageRevision(DisplayedPageRole role = DisplayedPageRole::Primary) const;

    void openPreviousImage();
    void openNextImage();
    void openPreviousSinglePage();
    void openNextSinglePage();
    void openImageAtPage(int pageNumber);
    void openPreviousContainer();
    void openNextContainer();
    void deleteDisplayedFile(FileDeletionMode mode);
    void resetZoom();
    void setFitMode(ImageZoomMode zoomMode);
    void updateRenderContext();

private:
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
    void dispatchEffects(ImageDocumentEffects effects);
    void setSourceUrlForLoad(const QUrl &sourceUrl, const QUrl &containerNavigationUrl,
        bool preserveTwoPageSpreadTransition = false);
    void clearAfterSuccessfulFileDeletion();
    void scheduleAdjacentImagePredecode();
    void cancelNavigationAndPredecode();
    void cancelPredecode();
    std::optional<PredecodedImage> takePredecodedImage(const QUrl &url) const;
    void finishWithAnimationError(const QString &errorString);
    void notify(ImageDocumentChange change);
    void clearImage();
    void clearSecondaryPage();
    void clearSecondaryPageAndNotify();
    void refreshSecondaryPage();
    void startSecondaryPageLoad(const QUrl &url);
    void finishSecondaryPredecodedImageLoad(ImageLoadSession session, PredecodedImage image);
    void finishSecondaryDecodedImageLoad(ImageLoadSession session, DecodedImage image);
    bool finishSecondaryDecodedImageResult(
        const ImageLoadSession &session, StaticDecodedImage &decoded);
    bool finishSecondaryDecodedImageResult(
        const ImageLoadSession &session, DecodedAnimationImage &decoded);
    bool finishSecondaryDecodedImageResult(
        const ImageLoadSession &session, ReaderAnimationImage &decoded);
    bool finishSecondaryDecodedImageResult(
        const ImageLoadSession &session, HeifSequenceAnimationImage &decoded);
    void finishSecondaryImageLoad(
        const ImageLoadSession &session, const QImage &image, bool predecodeCacheable);
    void finishSecondaryStaticImageLoad(
        const ImageLoadSession &session, StaticImagePayload staticImage, bool predecodeCacheable);
    void finishSecondaryLoadWithError(const ImageLoadSession &session);
    bool shouldBeginTwoPageSpreadTransition(int targetPageNumber) const;
    void beginTwoPageSpreadTransition();
    void finishTwoPageSpreadTransition();
    void notifyTwoPageSpreadTransitionChanged();
    void updateSpreadZoomState();
    void applyStoredSpreadZoomToPrimaryPage();
    void applySpreadVisibleItemRects();
    QRectF primarySpreadPageRect() const;
    QRectF secondarySpreadPageRect() const;
    QSize spreadImageSize() const;
    bool twoPageModeActive() const;
    bool currentPageIsCover() const;
    bool primaryPageIsWide() const;
    static bool pageIsWide(const QSize &imageSize);
    void cacheWidePage(const QUrl &url, const QSize &imageSize);
    std::optional<bool> cachedPageIsWide(const QUrl &url) const;
    static QString pageCacheKey(const QUrl &url);
    void openImageAtRelativePageOffset(int offset);
    void notifyTwoPageModeChanged();
    ImageDocumentRenderContext renderContext() const;
    qreal spreadDevicePixelRatio() const;

    ChangeCallback m_changeCallback;
    RenderContextProvider m_renderContextProvider;
    ImageDocumentState m_state;
    std::unique_ptr<ImageDeletionController> m_deletionController;
    std::unique_ptr<ImagePresentationController> m_presentationController;
    std::unique_ptr<ImagePresentationController> m_secondaryPresentationController;
    std::unique_ptr<ImageOpenController> m_openController;
    std::unique_ptr<ImageLoader> m_secondaryImageLoader;
    std::unique_ptr<ImageDocumentNavigationController> m_navigationController;
    std::unique_ptr<ImagePredecodeCoordinator> m_predecodeCoordinator;
    ImageZoomState m_spreadZoomState;
    DisplayedImageLocation m_secondaryDisplayedImageLocation;
    QRectF m_visibleItemRect;
    std::map<QString, bool> m_widePageByUrl;
    bool m_twoPageModeEnabled = false;
    bool m_secondaryPageVisible = false;
    bool m_twoPageSpreadTransitionInProgress = false;
};
}

#endif
