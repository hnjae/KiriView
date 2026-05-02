// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENCONTROLLER_H
#define KIRIVIEW_IMAGEOPENCONTROLLER_H

#include "imagedocumentevents.h"
#include "imageloader.h"

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <optional>

class QObject;

namespace KiriView {
class ImageDocumentState;
class ImagePresentationController;

class ImageOpenController final
{
public:
    using TakePredecodedImageCallback = std::function<std::optional<PredecodedImage>(const QUrl &)>;
    using EventCallback = std::function<void(DocumentEvent)>;

    ImageOpenController(QObject *parent, ImageDocumentState &state,
        ImagePresentationController &presentationController,
        TakePredecodedImageCallback takePredecodedImage, EventCallback eventCallback);
    ~ImageOpenController();

    void open();
    void cancel();
    void finishContainerNavigationWithEmptyContainer(const QUrl &containerUrl);
    void finishContainerNavigationLoadWithError(
        const QUrl &containerUrl, const QString &errorString);

private:
    void finishEmptySourceLoad();
    void beginSourceLoad();
    void setSourceUrlFromResolvedLoad(const QUrl &sourceUrl);
    void finishPredecodedImageLoad(ImageLoadSession session, const QImage &image);
    void finishDecodedImageLoad(
        ImageLoadSession session, std::shared_ptr<DecodedImageResult> result);
    void finishLoadWithError(
        const ImageLoadSession &session, ImageLoadError error, const QString &errorString);
    void finishReplacementLoadWithError(const QString &errorString);
    void finishInitialLoadWithError(const QString &errorString);
    void finishLoadSuccessfully(
        const ImageLoadSession &session, const QImage &image, bool predecodeCacheable);
    void finishSvgLoadSuccessfully(
        ImageLoadSession session, QByteArray data, const QSize &intrinsicSize);
    void prepareSuccessfulImageLoad(const ImageLoadSession &session);
    void finishSuccessfulImageLoad(const ImageLoadSession &session);
    void updateContainerNavigationFromDisplayedImage();
    void report(DocumentEvent event);

    ImageDocumentState &m_state;
    ImagePresentationController &m_presentationController;
    TakePredecodedImageCallback m_takePredecodedImage;
    EventCallback m_eventCallback;
    std::unique_ptr<ImageLoader> m_imageLoader;
};
}

#endif
