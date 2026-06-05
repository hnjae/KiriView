// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESECONDARYPAGECONTROLLER_H
#define KIRIVIEW_IMAGESECONDARYPAGECONTROLLER_H

#include "cache/imagecachepolicy.h"
#include "decoding/imagedecodedependencies.h"
#include "document/imagedocumenttypes.h"
#include "document/imageloadtypes.h"
#include "location/imagelocation.h"
#include "navigation/imagedocumentpagecandidateprovider.h"
#include "predecode/predecodedimage.h"
#include "presentation/imagepresentationload.h"
#include "presentation/imagepresentationruntime.h"
#include "presentation/imagesecondarypagestate.h"
#include "rendering/imagerendercontext.h"

#include <QSize>
#include <QSizeF>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <memory>
#include <optional>

class QObject;

namespace KiriView {
class ImageLoader;
class ImagePageSurfaceController;

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
        Callbacks callbacks, ImageDocumentPageCandidateProvider candidateProvider,
        ImageDecodeDependencies decodeDependencies, ImageCacheBudgets cacheBudgets);
    ~ImageSecondaryPageController();

    ImagePageSurfaceController &pageSurfaceController();
    const ImagePageSurfaceController &pageSurfaceController() const;
    bool visible() const;
    DisplayedImageLocation displayedImageLocation() const;
    QSize imageSize() const;
    ImagePresentationPageSlotSnapshot pageSlotSnapshot() const;

    void startLoad(const QUrl &url,
        const OpenedCollectionScopeLocation &displayedOpenedCollectionScope,
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
    ImageDocumentRenderContext renderContext() const;
    void notify(ImageDocumentChange change);

    Callbacks m_callbacks;
    RenderContextProvider m_renderContextProvider;
    std::unique_ptr<ImagePageSurfaceController> m_pageSurfaceController;
    std::unique_ptr<ImageLoader> m_imageLoader;
    ImageSecondaryPageState m_displayState;
};
}

#endif
