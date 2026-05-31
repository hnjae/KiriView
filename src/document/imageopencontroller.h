// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENCONTROLLER_H
#define KIRIVIEW_IMAGEOPENCONTROLLER_H

#include "decoding/imagedecodedependencies.h"
#include "imagedocumentruntimeplan.h"
#include "imageloadtypes.h"
#include "metadata/embeddedmetadata.h"
#include "navigation/imagedocumentpagecandidateprovider.h"
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
    using RuntimePlanCallback = std::function<void(const ImageDocumentRuntimePlan &)>;
    using UnsupportedOpenedCollectionVideoEnteredCallback = std::function<void(const QString &)>;

    struct Callbacks {
        FindPredecodedImageCallback findPredecodedImage;
        RuntimePlanCallback runtimePlan;
        UnsupportedOpenedCollectionVideoEnteredCallback unsupportedOpenedCollectionVideoEntered;
    };

    ImageOpenController(QObject *parent, ImageDocumentState &state,
        ImagePresentationController &presentationController, Callbacks callbacks,
        ImageDocumentPageCandidateProvider candidateProvider,
        ImageDecodeDependencies decodeDependencies);
    ~ImageOpenController();

    void open();
    void cancel();
    void finishEmptySourceLoad();
    void finishAnimationLoadWithError(const QString &errorString);
    void finishContainerNavigationWithEmptyContainer(const QUrl &containerUrl);
    void finishContainerNavigationLoadWithError(
        const QUrl &containerUrl, const QString &errorString);

private:
    void beginSourceLoad();
    void finishSourceResolved(ImageLoadSession session);
    void finishUnsupportedOpenedCollectionVideoLoad(ImageLoadSession session);
    void finishPredecodedImageLoad(ImageLoadSession session, PredecodedImage image);
    void finishDecodedImageLoad(ImageLoadSession session, DecodedImage image);
    void finishPresentedImageLoad(const ImageLoadSession &session,
        const ImagePresentationLoadResult &result, EmbeddedMetadata metadata);
    void finishLoadWithError(
        const ImageLoadSession &session, ImageLoadError error, const QString &errorString);
    void finishSuccessfulImageLoad(const ImageLoadSession &session);
    void reportRuntimePlan(ImageDocumentRuntimePlan plan);

    ImageDocumentState &m_state;
    ImagePresentationController &m_presentationController;
    Callbacks m_callbacks;
    std::unique_ptr<ImageLoader> m_imageLoader;
};
}

#endif
