// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENCONTROLLER_H
#define KIRIVIEW_IMAGEOPENCONTROLLER_H

#include "imageasyncdependencies.h"
#include "imagedocumenteffects.h"
#include "imageloader.h"
#include "imageopenworkflow.h"

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
    using EffectCallback = std::function<void(ImageDocumentEffect)>;

    struct Callbacks {
        TakePredecodedImageCallback takePredecodedImage;
        EffectCallback effect;
    };

    ImageOpenController(QObject *parent, ImageDocumentState &state,
        ImagePresentationController &presentationController, Callbacks callbacks,
        const ImageAsyncDependencies &dependencies);
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
    void finishDecodedImageResult(
        ImageLoadSession &session, DecodedImageFailure &decoded, const DecodedImageResult &result);
    void finishDecodedImageResult(
        ImageLoadSession &session, SvgDecodedImage &decoded, const DecodedImageResult &result);
    void finishDecodedImageResult(
        ImageLoadSession &session, StaticDecodedImage &decoded, const DecodedImageResult &result);
    void finishDecodedImageResult(ImageLoadSession &session, DecodedAnimationImage &decoded,
        const DecodedImageResult &result);
    void finishDecodedImageResult(
        ImageLoadSession &session, ReaderAnimationImage &decoded, const DecodedImageResult &result);
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
    void reportEffects(const ImageDocumentEffects &effects);
    void report(ImageDocumentEffect effect);

    ImageDocumentState &m_state;
    ImagePresentationController &m_presentationController;
    Callbacks m_callbacks;
    std::unique_ptr<ImageLoader> m_imageLoader;
};
}

#endif
