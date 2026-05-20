// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENCONTROLLER_H
#define KIRIVIEW_IMAGEOPENCONTROLLER_H

#include "decoding/imagedecodedependencies.h"
#include "imagedocumenteffects.h"
#include "imageloadtypes.h"
#include "navigation/imagecandidateprovider.h"
#include "predecode/predecodedimage.h"
#include "presentation/imagepresentationload.h"

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
    using FindPredecodedImageCallback = std::function<std::optional<PredecodedImage>(const QUrl &)>;
    using EffectCallback = std::function<void(ImageDocumentEffect)>;

    struct Callbacks {
        FindPredecodedImageCallback findPredecodedImage;
        EffectCallback effect;
    };

    ImageOpenController(QObject *parent, ImageDocumentState &state,
        ImagePresentationController &presentationController, Callbacks callbacks,
        ImageNavigationCandidateProvider candidateProvider,
        ImageDecodeDependencies decodeDependencies);
    ~ImageOpenController();

    void open();
    void cancel();
    void finishAnimationLoadWithError(const QString &errorString);
    void finishContainerNavigationWithEmptyContainer(const QUrl &containerUrl);
    void finishContainerNavigationLoadWithError(
        const QUrl &containerUrl, const QString &errorString);

private:
    void finishEmptySourceLoad();
    void beginSourceLoad();
    void setSourceUrlFromResolvedLoad(const QUrl &sourceUrl);
    void finishPredecodedImageLoad(ImageLoadSession session, PredecodedImage image);
    void finishDecodedImageLoad(ImageLoadSession session, DecodedImage image);
    void finishPresentedImageLoad(
        const ImageLoadSession &session, const ImagePresentationLoadResult &result);
    void finishLoadWithError(
        const ImageLoadSession &session, ImageLoadError error, const QString &errorString);
    void finishSuccessfulImageLoad(const ImageLoadSession &session);
    void reportEffects(ImageDocumentEffects effects);
    void report(ImageDocumentEffect effect);

    ImageDocumentState &m_state;
    ImagePresentationController &m_presentationController;
    Callbacks m_callbacks;
    std::unique_ptr<ImageLoader> m_imageLoader;
};
}

#endif
