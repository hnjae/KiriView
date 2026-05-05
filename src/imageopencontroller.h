// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENCONTROLLER_H
#define KIRIVIEW_IMAGEOPENCONTROLLER_H

#include "decodedimageresult.h"
#include "imageasyncdependencies.h"
#include "imagedocumenteffects.h"
#include "imageloadtypes.h"

#include <QImage>
#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <optional>

class QObject;

namespace KiriView {
class ImageDocumentState;
class ImageLoader;
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
    void finishPredecodedImageLoad(ImageLoadSession session, PredecodedImage image);
    void finishDecodedImageLoad(
        ImageLoadSession session, std::shared_ptr<DecodedImageResult> result);
    bool finishDecodedImageResult(
        ImageLoadSession &session, DecodedImageFailure &decoded, const DecodedImageResult &result);
    bool finishDecodedImageResult(
        ImageLoadSession &session, StaticDecodedImage &decoded, const DecodedImageResult &result);
    bool finishDecodedImageResult(ImageLoadSession &session, DecodedAnimationImage &decoded,
        const DecodedImageResult &result);
    bool finishDecodedImageResult(
        ImageLoadSession &session, ReaderAnimationImage &decoded, const DecodedImageResult &result);
    bool finishDecodedImageResult(ImageLoadSession &session, HeifSequenceAnimationImage &decoded,
        const DecodedImageResult &result);
    void finishLoadWithError(
        const ImageLoadSession &session, ImageLoadError error, const QString &errorString);
    void finishReplacementLoadWithError(const QString &errorString);
    void finishInitialLoadWithError(const QString &errorString);
    void finishStaticImageLoad(const ImageLoadSession &session,
        std::shared_ptr<ImageTileSource> source, const QImage &preview,
        StaticImageDisplayHints displayHints, bool predecodeCacheable);
    void finishLoadSuccessfully(
        const ImageLoadSession &session, const QImage &image, bool predecodeCacheable);
    void beginSuccessfulImagePresentation(const ImageLoadSession &session, bool predecodeCacheable);
    void finishSuccessfulImagePresentation(const ImageLoadSession &session);
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
