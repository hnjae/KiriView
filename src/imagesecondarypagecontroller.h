// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESECONDARYPAGECONTROLLER_H
#define KIRIVIEW_IMAGESECONDARYPAGECONTROLLER_H

#include "decoding/imagedecodedependencies.h"
#include "document/imagedocumenttypes.h"
#include "document/imageloadtypes.h"
#include "imagelocation.h"
#include "imagepresentationload.h"
#include "imagesecondarypagestate.h"
#include "navigation/imagecandidateprovider.h"
#include "predecode/predecodedimage.h"
#include "rendering/imagesurface.h"

#include <QSize>
#include <QSizeF>
#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <optional>

class QObject;

namespace KiriView {
class ImageLoader;
class ImagePresentationController;

class ImageSecondaryPageController final
{
public:
    using RenderContextProvider = std::function<ImageDocumentRenderContext()>;
    using ChangeCallback = std::function<void(ImageDocumentChange)>;
    using LoadFinishedCallback = std::function<void(
        ImageSecondaryPageLoadResult, const DisplayedImageLocation &, const QSize &)>;
    using VisibilityChangedCallback = std::function<void()>;
    using FindPredecodedImageCallback = std::function<std::optional<PredecodedImage>(const QUrl &)>;

    struct Callbacks {
        ChangeCallback change;
        LoadFinishedCallback loadFinished;
        VisibilityChangedCallback visibilityChanged;
        FindPredecodedImageCallback findPredecodedImage;
    };

    ImageSecondaryPageController(QObject *parent, RenderContextProvider renderContextProvider,
        Callbacks callbacks, ImageNavigationCandidateProvider candidateProvider,
        ImageDecodeDependencies decodeDependencies);
    ~ImageSecondaryPageController();

    ImagePresentationController &presentationController();
    const ImagePresentationController &presentationController() const;
    bool visible() const;
    DisplayedImageLocation displayedImageLocation() const;
    QSize imageSize() const;
    DisplayedImageRenderSnapshot renderSnapshot() const;

    void setViewportSize(const QSizeF &viewportSize);
    void updateRenderContext();
    void startLoad(const QUrl &url, const ArchiveDocumentLocation &displayedArchiveDocument,
        const ImageFirstDisplayDecodeContext &firstDisplayContext);
    void clear();
    void cancel();
    void stopAnimation();

private:
    void finishPredecodedImageLoad(ImageLoadSession session, PredecodedImage image);
    void finishDecodedImageLoad(ImageLoadSession session, DecodedImage image);
    void finishImagePresentation(
        const ImageLoadSession &session, const ImagePresentationLoadResult &result);
    void finishLoadWithError(const ImageLoadSession &session);
    void applyLoadCompletion(const ImageSecondaryPageLoadCompletion &completion);
    void notify(ImageDocumentChange change);

    Callbacks m_callbacks;
    std::unique_ptr<ImagePresentationController> m_presentationController;
    std::unique_ptr<ImageLoader> m_imageLoader;
    ImageSecondaryPageState m_displayState;
};
}

#endif
