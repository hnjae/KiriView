// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENCONTROLLER_H
#define KIRIVIEW_IMAGEOPENCONTROLLER_H

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
    using VoidCallback = std::function<void()>;

    ImageOpenController(QObject *parent, ImageDocumentState &state,
        ImagePresentationController &presentationController,
        TakePredecodedImageCallback takePredecodedImage, VoidCallback clearImage,
        VoidCallback updatePageNavigation, VoidCallback scheduleAdjacentImagePredecode);
    ~ImageOpenController();

    void open();
    void cancel();
    void finishContainerNavigationWithEmptyContainer(const QUrl &containerUrl);
    void finishContainerNavigationLoadWithError(
        const QUrl &containerUrl, const QString &errorString);

private:
    void setSourceUrlFromResolvedLoad(const QUrl &sourceUrl);
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
    void updateContainerNavigationFromDisplayedImage();

    ImageDocumentState &m_state;
    ImagePresentationController &m_presentationController;
    TakePredecodedImageCallback m_takePredecodedImage;
    VoidCallback m_clearImage;
    VoidCallback m_updatePageNavigation;
    VoidCallback m_scheduleAdjacentImagePredecode;
    std::unique_ptr<ImageLoader> m_imageLoader;
};
}

#endif
