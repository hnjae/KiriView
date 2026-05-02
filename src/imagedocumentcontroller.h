// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTCONTROLLER_H

#include "imageloader.h"
#include "imagezoomstate.h"

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <memory>
#include <optional>

namespace KiriView {
class DisplayedImageState;
class ImageNavigationService;
class ImagePredecodeCoordinator;
enum class NavigationDirection : int;

enum class ImageDocumentStatus {
    Null,
    Loading,
    Ready,
    Error,
};

enum class ImageDocumentChange {
    SourceUrl,
    Status,
    Loading,
    ErrorString,
    WindowTitleFileName,
    ImageSize,
    ViewportSize,
    DisplaySize,
    ZoomPercent,
    ZoomMode,
    PageNavigation,
    ContainerNavigation,
    Repaint,
};

struct ImageDocumentRenderContext {
    qreal devicePixelRatio = 1.0;
    int maximumTextureSize = 0;
};

class ImageDocumentController final : public QObject
{
public:
    using RenderContextProvider = std::function<ImageDocumentRenderContext()>;
    using ChangeCallback = std::function<void(ImageDocumentChange)>;

    ImageDocumentController(QObject *parent, RenderContextProvider renderContextProvider,
        ChangeCallback changeCallback);
    ~ImageDocumentController() override;

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl &sourceUrl);
    ImageDocumentStatus status() const;
    bool loading() const;
    QString errorString() const;
    QString windowTitleFileName() const;
    QSize imageSize() const;
    QSizeF viewportSize() const;
    void setViewportSize(const QSizeF &viewportSize);
    QSizeF displaySize() const;
    qreal zoomPercent() const;
    void setZoomPercent(qreal zoomPercent);
    ImageZoomMode zoomMode() const;
    int currentPageNumber() const;
    int imageCount() const;
    bool containerNavigationAvailable() const;
    const QImage &image() const;
    quint64 imageRevision() const;

    void openPreviousImage();
    void openNextImage();
    void openImageAtPage(int pageNumber);
    void openPreviousContainer();
    void openNextContainer();
    void resetZoom();
    void setFitMode(ImageZoomMode zoomMode);
    void updateRenderContext();

private:
    void setSourceUrlForLoad(const QUrl &sourceUrl, const QUrl &containerNavigationUrl);
    void startLoad();
    void cancelLoad();
    void setSourceUrlFromResolvedLoad(const QUrl &sourceUrl);
    void openAdjacentImage(NavigationDirection direction);
    void cancelNavigation();
    void openAdjacentContainer(NavigationDirection direction);
    void cancelContainerNavigation();
    void openImageFromContainerNavigation(const QUrl &imageUrl, const QUrl &containerUrl);
    void finishContainerNavigationWithEmptyContainer(const QUrl &containerUrl);
    void finishContainerNavigationLoadWithError(
        const QUrl &containerUrl, const QString &errorString);
    void setContainerNavigationUrl(const QUrl &containerUrl);
    void updateContainerNavigationFromDisplayedImage();
    void updatePageNavigation();
    void cancelPageNavigationUpdate();
    void clearPageNavigation();
    void scheduleAdjacentImagePredecode();
    void cancelPredecode();
    std::optional<PredecodedImage> takePredecodedImage(const QUrl &url) const;
    void finishPredecodedImageLoad(ImageLoadSession session, const QImage &image);
    void finishDecodedImageLoad(
        ImageLoadSession session, std::shared_ptr<DecodedImageResult> result);
    void finishLoadWithError(
        const ImageLoadSession &session, ImageLoadError error, const QString &errorString);
    void finishLoadSuccessfully(
        const ImageLoadSession &session, const QImage &image, bool predecodeCacheable);
    void finishSvgLoadSuccessfully(
        ImageLoadSession session, QByteArray data, const QSize &intrinsicSize);
    void prepareSuccessfulImageLoad(const ImageLoadSession &session);
    void finishSuccessfulImageLoad(const ImageLoadSession &session);
    bool hasDisplayedImage() const;
    void stopAnimation();
    void finishWithAnimationError(const QString &errorString);
    void setDisplayedImage(const QImage &image);
    void setDisplayedSvgImage(
        QByteArray data, const QSize &intrinsicSize, const QImage &image, const QSize &rasterSize);
    bool updateDisplayedSvgRaster();
    void setLoading(bool loading);
    void setStatus(ImageDocumentStatus status);
    void setErrorString(const QString &errorString);
    void setWindowTitleFileName(const QString &fileName);
    void updateWindowTitleFileName();
    void setImageSize(const QSize &imageSize);
    void applyZoomStateChanges(const ImageZoomSnapshot &previous);
    qreal displayDevicePixelRatio() const;
    int maximumTextureSize() const;
    ImageDocumentRenderContext renderContext() const;
    void notify(ImageDocumentChange change);
    void clearImage();

    RenderContextProvider m_renderContextProvider;
    ChangeCallback m_changeCallback;
    QUrl m_sourceUrl;
    QUrl m_displayedUrl;
    QUrl m_displayedComicBookRootUrl;
    ImageDocumentStatus m_status = ImageDocumentStatus::Null;
    bool m_loading = false;
    QString m_errorString;
    QString m_windowTitleFileName;
    ImageZoomState m_zoomState;
    std::unique_ptr<DisplayedImageState> m_displayedImageState;
    std::unique_ptr<ImageNavigationService> m_navigationService;
    std::unique_ptr<ImageLoader> m_imageLoader;
    std::unique_ptr<ImagePredecodeCoordinator> m_predecodeCoordinator;
    QUrl m_containerNavigationUrl;
    QUrl m_loadingContainerNavigationUrl;
};
}

#endif
