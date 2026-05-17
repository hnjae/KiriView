// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESECONDARYPAGECONTROLLER_H
#define KIRIVIEW_IMAGESECONDARYPAGECONTROLLER_H

#include "decodedimagepresentation.h"
#include "imageasyncdependencies.h"
#include "imagedocumenttypes.h"
#include "imageloadtypes.h"
#include "imagelocation.h"
#include "imagespreadpagecache.h"
#include "imagesurface.h"
#include "predecodedimage.h"

#include <QImage>
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

enum class ImageSecondaryPageLoadResult {
    PrimaryOnly,
    Visible,
    Failed,
};

class ImageSecondaryPageController final
{
public:
    using RenderContextProvider = std::function<ImageDocumentRenderContext()>;
    using ChangeCallback = std::function<void(ImageDocumentChange)>;
    using LoadFinishedCallback = std::function<void(
        ImageSecondaryPageLoadResult, const DisplayedImageLocation &, const QSize &)>;
    using VisibilityChangedCallback = std::function<void()>;
    using TakePredecodedImageCallback = std::function<std::optional<PredecodedImage>(const QUrl &)>;

    struct Callbacks {
        ChangeCallback change;
        LoadFinishedCallback loadFinished;
        VisibilityChangedCallback visibilityChanged;
        TakePredecodedImageCallback takePredecodedImage;
    };

    ImageSecondaryPageController(QObject *parent, RenderContextProvider renderContextProvider,
        Callbacks callbacks, ImageNavigationCandidateProvider candidateProvider,
        ImageDecodeDependencies decodeDependencies);
    ~ImageSecondaryPageController();

    ImagePresentationController &presentationController();
    const ImagePresentationController &presentationController() const;
    bool visible() const;
    const DisplayedImageLocation &displayedImageLocation() const;
    QSize imageSize() const;
    std::shared_ptr<DisplayedImageSurface> imageSurface() const;
    quint64 imageRevision() const;
    void cachePageSize(const QUrl &url, const QSize &imageSize);
    std::optional<bool> cachedPageIsWide(const QUrl &url) const;

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
    bool finishDecodedImagePresentation(
        const ImageLoadSession &session, DecodedStaticImagePresentation &presentation);
    bool finishDecodedImagePresentation(
        const ImageLoadSession &session, const DecodedAnimationImagePresentation &presentation);
    bool finishDecodedImagePresentation(
        const ImageLoadSession &session, const UnpresentableDecodedImage &presentation);
    void finishImageLoad(
        const ImageLoadSession &session, const QImage &image, bool predecodeCacheable);
    void finishStaticImageLoad(
        const ImageLoadSession &session, StaticImagePayload staticImage, bool predecodeCacheable);
    void prepareImagePresentation(const ImageLoadSession &session);
    void finishImagePresentation(const ImageLoadSession &session);
    void finishLoadWithError(const ImageLoadSession &session);
    void notify(ImageDocumentChange change);
    void reportLoadFinished(
        ImageSecondaryPageLoadResult result, const DisplayedImageLocation &location, QSize size);

    Callbacks m_callbacks;
    std::unique_ptr<ImagePresentationController> m_presentationController;
    std::unique_ptr<ImageLoader> m_imageLoader;
    ImageSpreadPageCache m_pageCache;
    DisplayedImageLocation m_displayedImageLocation;
    bool m_visible = false;
};
}

#endif
