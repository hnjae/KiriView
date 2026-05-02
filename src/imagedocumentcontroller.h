// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTCONTROLLER_H

#include "imagedocumentstate.h"
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
class ImageNavigationService;
class ImageOpenController;
class ImagePresentationController;
class ImagePredecodeCoordinator;
enum class NavigationDirection : int;

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
    void cancelLoad();
    void openAdjacentImage(NavigationDirection direction);
    void cancelNavigation();
    void openAdjacentContainer(NavigationDirection direction);
    void cancelContainerNavigation();
    void openImageFromContainerNavigation(const QUrl &imageUrl, const QUrl &containerUrl);
    void setContainerNavigationUrl(const QUrl &containerUrl);
    void updatePageNavigation();
    void cancelPageNavigationUpdate();
    void clearPageNavigation();
    void scheduleAdjacentImagePredecode();
    void cancelPredecode();
    std::optional<PredecodedImage> takePredecodedImage(const QUrl &url) const;
    bool hasDisplayedImage() const;
    void stopAnimation();
    void finishWithAnimationError(const QString &errorString);
    void setLoading(bool loading);
    void setStatus(ImageDocumentStatus status);
    void setErrorString(const QString &errorString);
    void setWindowTitleFileName(const QString &fileName);
    void updateWindowTitleFileName();
    void notify(ImageDocumentChange change);
    void clearImage();

    RenderContextProvider m_renderContextProvider;
    ChangeCallback m_changeCallback;
    ImageDocumentState m_state;
    std::unique_ptr<ImagePresentationController> m_presentationController;
    std::unique_ptr<ImageOpenController> m_openController;
    std::unique_ptr<ImageNavigationService> m_navigationService;
    std::unique_ptr<ImagePredecodeCoordinator> m_predecodeCoordinator;
};
}

#endif
